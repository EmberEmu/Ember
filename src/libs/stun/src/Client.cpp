/*
 * Copyright (c) 2023 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stun/Client.h>
#include <stun/Protocol.h>
#include <stun/Transport.h>
#include <spark/buffers/BinaryInStream.h>
#include <spark/buffers/BinaryOutStream.h>
#include <spark/buffers/VectorBufferAdaptor.h>
#include <shared/util/FNVHash.h>
#include <boost/asio.hpp>
#include <boost/endian.hpp>
#include <stdexcept>
#include <utility>
#include <vector>
#include <cstddef>

using namespace std::chrono_literals;

template<class>
inline constexpr bool always_false_v = false;

namespace ember::stun {

Client::Client(std::unique_ptr<Transport> transport, RFCMode mode)
	: transport_(std::move(transport)), parser_(mode), mode_(mode), mt_(rd_()) {
	// should probably do this in the transport but whatever
	work_.emplace_back(std::make_shared<boost::asio::io_context::work>(*transport_->executor()));
	worker_ = std::jthread(static_cast<size_t(boost::asio::io_context::*)()>
		(&boost::asio::io_context::run), transport_->executor());
}

Client::~Client() {
	work_.clear();
	transport_.reset();
}

void Client::log_callback(LogCB callback, const Verbosity verbosity) {
	if(!callback) {
		throw std::invalid_argument("Logging callback cannot be null");
	}

	logger_ = callback;
	verbosity_ = verbosity;
	parser_.set_logger(callback, verbosity);
}

void Client::connect() {
	transport_->set_callbacks(
		[this](std::vector<std::uint8_t> buffer) { handle_response(std::move(buffer)); },
		[this](const boost::system::error_code& ec) { on_connection_error(ec); }
	);

	transport_->connect();
}

void Client::binding_request(detail::Transaction& tx) {
	std::vector<std::uint8_t> data;
	spark::VectorBufferAdaptor buffer(data);
	spark::BinaryOutStream stream(buffer);

	Header header{ };
	header.type = std::to_underlying(MessageType::BINDING_REQUEST);
	stream << header.type;
	stream << header.length;
	header.cookie = MAGIC_COOKIE;

	if(mode_ == RFCMode::RFC5389) {
		stream << header.cookie;
		stream.put(tx.tx_id.id_5389.begin(), tx.tx_id.id_5389.end());
	} else {
		stream.put(tx.tx_id.id_3489.begin(), tx.tx_id.id_3489.end());
	}
	
	transaction_timer(tx);
	transport_->send(data);
}

detail::Transaction& Client::start_transaction(detail::Transaction::VariantPromise vp) {
	detail::Transaction tx(transport_->executor()->get_executor(),
		transport_->timeout(), transport_->retries());
	tx.promise = std::move(vp);

	if(mode_ == RFCMode::RFC5389) {
		for(auto& ele : tx.tx_id.id_5389) {
			ele = mt_();
		}
	} else {
		for(auto& ele : tx.tx_id.id_3489) {
			ele = mt_();
		}
	}

	tx.hash = tx_hash(tx.tx_id);
	auto entry = transactions_.emplace(tx.hash, std::move(tx));
	return entry.first->second;
}

void Client::handle_response(std::vector<std::uint8_t> buffer) try {
	if(buffer.size() < HEADER_LENGTH) {
		logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_BUFFER_LT_HEADER);
		return; // RFC says invalid messages should be discarded
	}

	spark::VectorBufferAdaptor<std::uint8_t> vba(buffer);
	spark::BinaryInStream stream(vba);

	const Header& header = parser_.header_from_stream(stream);

	// Check to see whether this is a response that we're expecting
	const auto hash = tx_hash(header.tx_id);

	if(!transactions_.contains(hash)) {
		logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_TX_NOT_FOUND);
		return;
	}

	detail::Transaction& transaction = transactions_.at(hash);
	transaction.timer.cancel();

	if(auto error = parser_.validate_header(header); error != Error::OK) {
		abort_transaction(transaction, error);
		return;
	}

	MessageType type{ static_cast<std::uint16_t>(header.type) };
	process_transaction(stream, transaction, type);
} catch(const spark::exception& e) {
	logger_(Verbosity::STUN_LOG_DEBUG, Error::BUFFER_PARSE_ERROR);
}

void Client::abort_transaction(detail::Transaction& tx, const Error error) {
	std::visit([&](auto&& arg) {
		using T = std::decay_t<decltype(arg)>;

		if constexpr(std::is_same_v<T,
			std::promise<std::expected<attributes::MappedAddress, Error>>>) {
			arg.set_value(std::unexpected(error));
		} else if constexpr(std::is_same_v<T,
			std::promise<std::expected<std::vector<attributes::Attribute>, Error>>>) {
			arg.set_value(std::unexpected(error));
		} else {
			static_assert(always_false_v<T>, "Unhandled variant type");
		}
	}, tx.promise);

	transactions_.erase(tx.hash);
}

void Client::process_transaction(spark::BinaryInStream& stream, detail::Transaction& tx,
                                 const MessageType type) try {
	auto attributes = parser_.extract_attributes(stream, tx.tx_id, type);
	complete_transaction(tx, std::move(attributes));
} catch (const spark::exception&) {
	abort_transaction(tx, Error::BUFFER_PARSE_ERROR);
} catch (const Error& e) {
	abort_transaction(tx, e);
}

void Client::complete_transaction(detail::Transaction& tx, std::vector<attributes::Attribute> attributes) {
	// figure out which attributes we care about
	std::visit([&](auto&& arg) {
		using T = std::decay_t<decltype(arg)>;

		if constexpr(std::is_same_v<T,
			std::promise<std::expected<attributes::MappedAddress, Error>>>) {
			for(const auto& attr : attributes) {
				if(std::holds_alternative<attributes::MappedAddress>(attr)) {
					arg.set_value(std::get<attributes::MappedAddress>(attr));
					return;
				}

				// XorMappedAddress will also do - we just need an external address
				if (std::holds_alternative<attributes::XorMappedAddress>(attr)) {
					const auto xma = std::get<attributes::XorMappedAddress>(attr);

					const attributes::MappedAddress ma{
						.family = xma.family,
						.ipv4 = xma.ipv4,
						.ipv6 = xma.ipv6,
						.port = xma.port
					};

					arg.set_value(ma);
				}
			}
		} else if constexpr(std::is_same_v<T,
			std::promise<std::expected<std::vector<attributes::Attribute>, Error>>>) {
			arg.set_value(std::move(attributes));
		} else {
			static_assert(always_false_v<T>, "Unhandled variant type");
		}
	}, tx.promise);

	transactions_.erase(tx.hash);
}

std::future<std::expected<std::vector<attributes::Attribute>, Error>> 
Client::binding_request() {
	std::promise<std::expected<std::vector<attributes::Attribute>, Error>> promise;
	auto future = promise.get_future();
	detail::Transaction::VariantPromise vp(std::move(promise));
	auto& tx = start_transaction(std::move(vp));
	binding_request(tx);
	return future;
}

std::future<std::expected<attributes::MappedAddress, Error>>
Client::external_address() {
	std::promise<std::expected<attributes::MappedAddress, Error>> promise;
	auto future = promise.get_future();
	detail::Transaction::VariantPromise vp(std::move(promise));
	auto& tx = start_transaction(std::move(vp));
	binding_request(tx);
	return future;
}

std::size_t Client::tx_hash(const TxID& tx_id) {
	/*
	 * Hash the transaction ID to use as a key for future lookup.
	 * FNV is used because it's already in the project, not for any
	 * particular property. Odds of a collision are very low. 
	 */
	FNVHash fnv;

	if(mode_ == RFCMode::RFC5389) {
		fnv.update(tx_id.id_5389.begin(), tx_id.id_5389.end());
	} else {
		fnv.update(tx_id.id_3489.begin(), tx_id.id_3489.end());
	}

	return fnv.hash();
}

void Client::transaction_timer(detail::Transaction& tx) {
	tx.timer.expires_from_now(tx.timeout);
	tx.timer.async_wait([&, hash = tx.hash](const boost::system::error_code& ec) {
		if(ec) {
			return;
		}

		if(auto entry = transactions_.find(hash); entry != transactions_.end()) {
			auto& tx = entry->second;

			// RFC algorithm decided by interpretive dance
			if(tx.retries_left) {
				if (tx.retries_left != tx.max_retries) {
					tx.timeout *= 2;
				}
				
				--tx.retries_left;

				if(!tx.retries_left) {
					tx.timeout = tx.initial_to * TX_RM;
				}

				binding_request(tx);
			} else {
				abort_transaction(tx, Error::NO_RESPONSE_RECEIVED);
			}
		}
	});
}

void Client::on_connection_error(const boost::system::error_code& ec) {
	for (auto& [k, v] : transactions_) {
		if(ec == boost::asio::error::connection_aborted) {
			abort_transaction(v, Error::CONNECTION_ABORTED);
		} else if (ec == boost::asio::error::connection_reset) {
			abort_transaction(v, Error::CONNECTION_RESET);
		} else {
			abort_transaction(v, Error::CONNECTION_ERROR);
		}
	}
}

} // stun, ember