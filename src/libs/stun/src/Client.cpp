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
#include <stun/Exception.h>
#include <spark/buffers/BinaryInStream.h>
#include <spark/buffers/BinaryOutStream.h>
#include <spark/buffers/VectorBufferAdaptor.h>
#include <shared/util/FNVHash.h>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/endian.hpp>
#include <stdexcept>
#include <utility>
#include <vector>
#include <cstddef>

using namespace std::chrono_literals;

template<class>
inline constexpr bool always_false_v = false;

namespace ember::stun {

Client::Client(std::unique_ptr<Transport> transport, std::string host,
               const std::uint16_t port, RFCMode mode)
	: transport_(std::move(transport)),
      host_(std::move(host)), port_(port),
      mode_(mode), mt_(rd_()) {
	transport_->set_callbacks(
		[this](std::vector<std::uint8_t> buffer) { handle_message(std::move(buffer)); },
		[this](const boost::system::error_code& ec) { on_connection_error(ec); }
	);

	// should probably do this in the transport but whatever
	work_.emplace_back(std::make_shared<boost::asio::io_context::work>(*transport_->executor()));
	worker_ = std::jthread(static_cast<size_t(boost::asio::io_context::*)()>
		(&boost::asio::io_context::run), transport_->executor());
}

Client::~Client() {
	transport_->executor()->stop();
	work_.clear();
}

void Client::log_callback(LogCB callback, const Verbosity verbosity) {
	if(!callback) {
		throw exception(Error::BAD_CALLBACK, "Logging callback cannot be null");
	}

	logger_ = callback;
	verbosity_ = verbosity;
}

void Client::connect(const std::string& host, const std::uint16_t port, Transport::OnConnect cb) {
	dest_hist_[host] = std::chrono::steady_clock::now();
	transport_->connect(host, port, cb);
}

void Client::binding_request(detail::Transaction& tx) {
	std::lock_guard<std::mutex> guard(mutex_);

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
						   transport_->timeout(),
						   transport_->retries());

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

void Client::handle_message(std::vector<std::uint8_t> buffer) try {
	std::lock_guard<std::mutex> guard(mutex_);

	if(buffer.size() < HEADER_LENGTH) {
		logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_BUFFER_LT_HEADER);
		return; // RFC says invalid messages should be discarded
	}

	Parser parser(buffer, mode_);
	parser.set_logger(logger_, verbosity_);

	const Header& header = parser.header();

	// Check to see whether this is a response that we're expecting
	const auto hash = tx_hash(header.tx_id);

	if(!transactions_.contains(hash)) {
		logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_TX_NOT_FOUND);
		return;
	}

	detail::Transaction& transaction = transactions_.at(hash);
	transaction.timer.cancel();

	if(auto error = parser.validate_header(header); error != Error::OK) {
		abort_transaction(transaction, error);
		return;
	}

	process_message(buffer, transaction);
} catch(const spark::exception& e) {
	logger_(Verbosity::STUN_LOG_DEBUG, Error::BUFFER_PARSE_ERROR);
}

void Client::process_message(const std::vector<std::uint8_t>& buffer,
                             detail::Transaction& tx) try {
	Parser parser(buffer, mode_);
	parser.set_logger(logger_, verbosity_);
	const auto header = parser.header();
	const auto attributes = parser.attributes();

	if(const auto fp = retrieve_attribute<attributes::Fingerprint>(attributes)) {
		const auto crc32 = parser.calculate_fingerprint();

		if(crc32 != fp->crc32) {
			abort_transaction(tx, Error::RESP_INVALID_FINGERPRINT);
			return;
		}
	}

	const MessageType type{ static_cast<std::uint16_t>(header.type) };

	if(type == MessageType::BINDING_RESPONSE) {
		handle_binding_resp(std::move(attributes), tx);
	} else if(type == MessageType::BINDING_ERROR_RESPONSE) {
		handle_binding_err_resp(attributes, tx);
	} else {
		abort_transaction(tx, Error::RESP_UNK_MESSAGE_TYPE);
	}
} catch (const spark::exception&) {
	abort_transaction(tx, Error::BUFFER_PARSE_ERROR);
} catch (const Error& e) {
	abort_transaction(tx, e);
}

void Client::abort_transaction(detail::Transaction& tx, const Error error,
                               attributes::ErrorCode ec, const bool erase) {
	std::visit([&](auto&& arg) {
		using T = std::decay_t<decltype(arg)>;
		arg.set_value(std::unexpected(ErrorRet(error)));
	}, tx.promise);

	if(erase) {
		transactions_.erase(tx.hash);
	}
}

void Client::set_nat_present(const std::vector<attributes::Attribute>& attributes) {
	attributes::MappedAddress mapped{};
	const auto xma = retrieve_attribute<attributes::XorMappedAddress>(attributes);

	// we don't care if it's a mapped or xormapped attribute, as long we have one
	if(xma) {
		mapped.family = xma->family;
		mapped.ipv4 = xma->ipv4;
		mapped.ipv6 = xma->ipv6;
		mapped.port = xma->port;
	} else {
		const auto ma = retrieve_attribute<attributes::MappedAddress>(attributes);

		if(!ma) {
			return;
		}

		mapped = *ma;
	}
	
	const std::string& bind_ip = extract_ip_to_string(mapped);
	is_nat_present_ = (transport_->local_ip() != bind_ip);
}

void Client::handle_binding_resp(std::vector<attributes::Attribute> attributes,
                                 detail::Transaction& tx) {
	set_nat_present(attributes);
	complete_transaction(tx, std::move(attributes));
}

void Client::handle_binding_err_resp(const std::vector<attributes::Attribute>& attributes,
                                     detail::Transaction& tx) {
	const auto& ec = retrieve_attribute<attributes::ErrorCode>(attributes);

	if(!ec) {
		abort_transaction(tx, Error::RESP_MISSING_ATTR);
		return;
	}

	if(Errors(ec->code) == Errors::TRY_ALTERNATE) {
		// insurance policy against being bounced around servers
		if(tx.redirects >= MAX_REDIRECTS) {
			abort_transaction(tx, Error::RESP_BAD_REDIRECT);
			return;
		}

		++tx.redirects;

		const auto alt_attr = retrieve_attribute<attributes::AlternateServer>(attributes);

		if(!alt_attr) {
			abort_transaction(tx, Error::RESP_MISSING_ATTR);
			return;
		}

		/*
		 * RFC states "The IP address family MUST be identical
		 * to that of the source IP address of the request".
		 * I don't have a reliable way of doing this, so going to ignore it.
		 */
		const std::string& ip = extract_ip_to_string(*alt_attr);

		// check we haven't resent to this address previously to prevent a redirect loop
		if(auto entry = dest_hist_.find(ip); entry != dest_hist_.end()) {
			if(std::chrono::steady_clock::now() - entry->second < 5min) {
				abort_transaction(tx, Error::RESP_BAD_REDIRECT);
				return;
			}
		}

		// start a new transaction to try to fulfill the request
		auto& new_tx = start_transaction(std::move(tx.promise));
		new_tx.redirects = tx.redirects; // persist redirect counter
		transactions_.erase(tx.hash);

		// retry the request
		transport_->connect(ip, alt_attr->port, [&]() {
			binding_request(new_tx);
		});
	} else {
		if(ec) {
			abort_transaction(tx, Error::RESP_BINDING_ERROR, *ec);
		} else {
			abort_transaction(tx, Error::RESP_BINDING_ERROR);
		}
	}
}

void Client::complete_transaction(detail::Transaction& tx,
                                  std::vector<attributes::Attribute> attributes) {
	std::visit([&](auto&& arg) {
		using T = std::decay_t<decltype(arg)>;

		if constexpr(std::is_same_v<T,
			std::promise<std::expected<attributes::MappedAddress, ErrorRet>>>) {
			for(const auto& attr : attributes) {
				if(std::holds_alternative<attributes::MappedAddress>(attr)) {
					arg.set_value(std::get<attributes::MappedAddress>(attr));
					return;
				}

				// XorMappedAddress will also do - we just need an external address
				if(std::holds_alternative<attributes::XorMappedAddress>(attr)) {
					const auto& xma = std::get<attributes::XorMappedAddress>(attr);

					const attributes::MappedAddress ma{
						.ipv4 = xma.ipv4,
						.ipv6 = xma.ipv6,
						.port = xma.port,
						.family = xma.family
					};

					arg.set_value(ma);
					return;
				}
			}

			arg.set_value(std::unexpected(ErrorRet(Error::RESP_MISSING_ATTR)));
		} else if constexpr(std::is_same_v<T,
			std::promise<std::expected<std::vector<attributes::Attribute>, ErrorRet>>>) {
			arg.set_value(std::move(attributes));
		} else if constexpr(std::is_same_v<T,
			std::promise<std::expected<NAT, ErrorRet>>>) {
			// todo
		} else if constexpr(std::is_same_v<T, std::promise<std::expected<bool, ErrorRet>>>) {
			if(is_nat_present_) {
				arg.set_value(*is_nat_present_);
			} else {
				arg.set_value(std::unexpected(ErrorRet(Error::RESP_MISSING_ATTR)));
			}
		} else {
			static_assert(always_false_v<T>, "Unhandled variant type");
		}
	}, tx.promise);

	transactions_.erase(tx.hash);
}

template<typename T>
std::future<T> Client::basic_request() {
	std::lock_guard<std::mutex> guard(mutex_);
	std::promise<T> promise;
	auto future = promise.get_future();
	detail::Transaction::VariantPromise vp(std::move(promise));
	auto& tx = start_transaction(std::move(vp));

	connect(host_, port_, [&]() {
		binding_request(tx);
	});

	return future;
}

auto Client::binding_request() -> std::future<std::expected<std::vector<attributes::Attribute>, ErrorRet>> {
	return basic_request<std::expected<std::vector<attributes::Attribute>, ErrorRet>>();
}

auto Client::external_address() -> std::future<std::expected<attributes::MappedAddress, ErrorRet>> {
	return basic_request<std::expected<attributes::MappedAddress, ErrorRet>>();
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

		std::lock_guard<std::mutex> guard(mutex_);

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
	std::lock_guard<std::mutex> guard(mutex_);

	for(auto& [k, v] : transactions_) {
		if(ec == boost::asio::error::connection_aborted) {
			abort_transaction(v, Error::CONNECTION_ABORTED, {}, false);
		} else if(ec == boost::asio::error::connection_reset) {
			abort_transaction(v, Error::CONNECTION_RESET, {}, false);
		} else {
			abort_transaction(v, Error::CONNECTION_ERROR, {}, false);
		}
	}

	transactions_.clear();
}

std::future<std::expected<NAT, ErrorRet>> Client::nat_type() {
	std::lock_guard<std::mutex> guard(mutex_);
	std::promise<std::expected<NAT, ErrorRet>> promise;
	auto future = promise.get_future();
	detail::Transaction::VariantPromise vp(std::move(promise));
	auto& tx = start_transaction(std::move(vp));
	
	// todo

	return future;
}

std::future<std::expected<bool, ErrorRet>> Client::nat_present() {
	std::lock_guard<std::mutex> guard(mutex_);
	std::promise<std::expected<bool, ErrorRet>> promise;
	auto future = promise.get_future();

	if(is_nat_present_) {
		promise.set_value(*is_nat_present_);
		return future;
	}

	detail::Transaction::VariantPromise vp(std::move(promise));
	auto& tx = start_transaction(std::move(vp));
	
	connect(host_, port_, [&]() {
		binding_request(tx);
	});

	return future;
}

} // stun, ember