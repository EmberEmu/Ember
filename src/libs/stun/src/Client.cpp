/*
 * Copyright (c) 2023 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stun/Client.h>
#include <stun/Protocol.h>
#include <stun/DatagramTransport.h>
#include <stun/StreamTransport.h>
#include <spark/buffers/BinaryStream.h>
#include <spark/buffers/VectorBufferAdaptor.h>
#include <shared/util/FNVHash.h>
#include <boost/assert.hpp>
#include <boost/asio.hpp>
#include <boost/endian.hpp>
#include <stdexcept>
#include <utility>
#include <vector>
#include <cstddef>

#include <iostream> // todo temp

template<class>
inline constexpr bool always_false_v = false;

namespace ember::stun {

Client::Client(RFCMode mode) : mode_(mode), mt_(rd_()) {
	work_.emplace_back(std::make_shared<boost::asio::io_context::work>(ctx_));
	worker_ = std::jthread(static_cast<size_t(boost::asio::io_context::*)()>
		(&boost::asio::io_context::run), &ctx_);
}

Client::~Client() {
	work_.clear();
	transport_.reset();
	ctx_.stop();
}

void Client::log_callback(LogCB callback, const Verbosity verbosity) {
	if(!callback) {
		throw std::invalid_argument("Logging callback cannot be null");
	}

	logger_ = callback;
	verbosity_ = verbosity;
}

void Client::connect(const std::string& host, const std::uint16_t port, const Protocol protocol) {
	transport_.reset();

	switch(protocol) {
	case Protocol::UDP:
		transport_ = std::make_unique<DatagramTransport>(ctx_, host, port,
			[this](std::vector<std::uint8_t> buffer) { handle_response(std::move(buffer)); });
		break;
	case Protocol::TCP:
		transport_ = std::make_unique<StreamTransport>(ctx_, host, port,
			[this](std::vector<std::uint8_t> buffer) { handle_response(std::move(buffer)); });
		break;
	case Protocol::TLS_TCP:
		throw std::runtime_error("TLS_TCP STUN isn't handled yet");
	default:
		throw std::invalid_argument("Unknown protocol value specified");
	}

	transport_->connect();
}

void Client::binding_request(detail::Transaction::VariantPromise vp) {
	std::vector<std::uint8_t> data;
	spark::VectorBufferAdaptor buffer(data);
	spark::BinaryOutStream stream(buffer);

	Header header{ };
	header.type = std::to_underlying(MessageType::BINDING_REQUEST);

	if(mode_ == RFCMode::RFC5389) {
		header.cookie = MAGIC_COOKIE;

		for(auto& ele : header.tx_id_5389) {
			ele = mt_();
		}
	} else {
		for(auto& ele : header.tx_id_3489) {
			ele = mt_();
		}
	}

	stream << header.type;
	stream << header.length;

	if(mode_ == RFCMode::RFC5389) {
		stream << header.cookie;
		stream.put(header.tx_id_5389.begin(), header.tx_id_5389.end());
	} else {
		stream.put(header.tx_id_3489.begin(), header.tx_id_3489.end());
	}

	detail::Transaction transaction{};

	if(mode_ == RFCMode::RFC5389) {
		std::copy(header.tx_id_5389.begin(), header.tx_id_5389.end(),
			transaction.tx_id.begin());
	} else {
		std::copy(header.tx_id_3489.begin(), header.tx_id_3489.end(),
			transaction.tx_id.begin());
	}

	transaction.promise = std::move(vp);
	const auto hash = header_hash(header);
	transactions_[hash] = std::move(transaction);

	transport_->send(data);
}

void Client::handle_response(std::vector<std::uint8_t> buffer) try {
	if(buffer.size() < HEADER_LENGTH) {
		logger_(Verbosity::STUN_LOG_DEBUG, LogReason::RESP_BUFFER_LT_HEADER);
		return; // RFC says invalid messages should be discarded
	}

	spark::VectorBufferAdaptor<std::uint8_t> vba(buffer);
	spark::BinaryInStream stream(vba);

	Header header{};
	stream >> header.type;
	stream >> header.length;

	if(mode_ == RFCMode::RFC5389) {
		stream >> header.cookie;
		stream.get(header.tx_id_5389.begin(), header.tx_id_5389.end());
	} else {
		stream.get(header.tx_id_3489.begin(), header.tx_id_3489.end());
	}

	if(mode_ == RFCMode::RFC5389 && header.cookie != MAGIC_COOKIE) {
		logger_(Verbosity::STUN_LOG_DEBUG, LogReason::RESP_COOKIE_MISSING);
		return;
	}

	if(header.length < ATTR_HEADER_LENGTH) {
		logger_(Verbosity::STUN_LOG_DEBUG, LogReason::RESP_BAD_HEADER_LENGTH);
		return;
	}

	// Check to see whether this is a response that we're expecting
	const auto hash = header_hash(header);

	if(!transactions_.contains(hash)) {
		logger_(Verbosity::STUN_LOG_DEBUG, LogReason::RESP_TX_NOT_FOUND);
		return;
	}

	detail::Transaction& transaction = transactions_[hash];

	MessageType type{ static_cast<std::uint16_t>(header.type) };

	if(type == MessageType::BINDING_ERROR_RESPONSE) {
		handle_error_response(stream);
		// todo
	}

	if(type != MessageType::BINDING_RESPONSE) {
		// todo
	}

	const auto attributes = handle_attributes(stream, transaction);

	// figure out which attributes we care about
	std::visit([&](auto&& arg) {
		using T = std::decay_t<decltype(arg)>;

		if constexpr(std::is_same_v<T,
			std::promise<std::expected<attributes::MappedAddress, LogReason>>>) {
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
					return;
				}
			}
		} else if constexpr(std::is_same_v<T,
			std::promise<std::expected<std::vector<attributes::Attribute>,LogReason>>>) {
			arg.set_value(attributes);
		} else {
			static_assert(always_false_v<T>, "Unhandled variant type");
		}
	}, transaction.promise);
} catch(const std::exception& e) {
	std::cout << e.what(); // temp
}

void Client::handle_error_response(spark::BinaryInStream& stream) {

}

std::optional<attributes::Attribute> Client::extract_attribute(spark::BinaryInStream& stream,
                                                               detail::Transaction& tx) {
	Attributes attr_type;
	be::big_uint16_t length;
	stream >> attr_type;
	stream >> length;

	be::big_to_native_inplace(attr_type);

	attributes::Attribute attribute;

	/*
	 * If this attribute is marked as required, we'll look it up in the map
	 * to check whether we know what it is and more importantly whose fault
	 * it is if we can't finish parsing the message, given our current RFC mode
	 */
	const bool required = (std::to_underlying(attr_type) >> 15) ^ 1;

	if(required) {
		// might be our fault but probably not
		if(!attr_req_lut.contains(attr_type)) {
			logger_(Verbosity::STUN_LOG_DEBUG, LogReason::RESP_UNKNOWN_REQ_ATTRIBUTE);
			throw std::exception("todo"); // todo, abort all parsing here
		}

		const auto rfc = attr_req_lut[attr_type];
		
		// definitely not our fault... probably
		if(!(rfc & mode_)) {
			logger_(Verbosity::STUN_LOG_DEBUG, LogReason::RESP_BAD_REQ_ATTR_SERVER);
			throw std::exception("todo"); // todo, abort all parsing here
		}
	}

	// todo, actual error handling
	switch(attr_type) {
		case Attributes::MAPPED_ADDRESS:
			return handle_mapped_address(stream);
			break;
		case Attributes::XOR_MAPPED_ADDR_OPT:
			[[fallthrough]];
		case Attributes::XOR_MAPPED_ADDRESS:
			return handle_xor_mapped_address(stream, tx);
			break;
		case Attributes::OTHER_ADDRESS:
			// todo
			break;
		/*case Attributes::RESPONSE_ORIGIN:
			break;*/
	}

	stream.skip(length);

	logger_(Verbosity::STUN_LOG_DEBUG, required?
		LogReason::RESP_UNKNOWN_REQ_ATTRIBUTE : LogReason::RESP_UNKNOWN_OPT_ATTRIBUTE);

	// todo, error handling
	return std::nullopt;
}

std::vector<attributes::Attribute>
Client::handle_attributes(spark::BinaryInStream& stream, detail::Transaction& tx) {
	std::vector<attributes::Attribute> attributes;

	while(!stream.empty()) {
		auto attribute = extract_attribute(stream, tx);

		if(attribute) {
			attributes.emplace_back(std::move(*attribute));
		}
	}

	return attributes;
}

attributes::MappedAddress Client::handle_mapped_address(spark::BinaryInStream& stream) {
	stream.skip(1); // skip reserved byte
	attributes::MappedAddress attr{};
	stream >> attr.family;
	stream >> attr.port;

	if(attr.family == AddressFamily::IPV4) {
		attr.family = AddressFamily::IPV4;
		be::big_to_native_inplace(attr.port);
		stream >> attr.ipv4;
		be::big_to_native_inplace(attr.ipv4);
	} else if(attr.family == AddressFamily::IPV6) {
		if(mode_ == RFCMode::RFC3489) {
			logger_(Verbosity::STUN_LOG_DEBUG, LogReason::RESP_IPV6_NOT_VALID);
			throw std::exception("todo"); // todo
		}

		stream.get(attr.ipv6.begin(), attr.ipv6.end());
		
		for(auto& bytes : attr.ipv6) {
			be::big_to_native_inplace(bytes);
		}
	} else {
		logger_(Verbosity::STUN_LOG_DEBUG, LogReason::RESP_ADDR_FAM_NOT_VALID);
		throw std::exception("todo"); // todo
	}
	
	return attr;
}

attributes::XorMappedAddress
Client::handle_xor_mapped_address(spark::BinaryInStream& stream, const detail::Transaction& tx) {
	stream.skip(1); // skip reserved byte
	attributes::XorMappedAddress attr{};
	stream >> attr.family;

	// XOR port with the magic cookie
	stream >> attr.port;
	be::big_to_native_inplace(attr.port);
	attr.port ^= MAGIC_COOKIE >> 16;

	if(attr.family == AddressFamily::IPV4) {
		stream >> attr.ipv4;
		be::big_to_native_inplace(attr.ipv4);
		attr.ipv4 ^= MAGIC_COOKIE;
	} else if(attr.family == AddressFamily::IPV6) {
		stream.get(attr.ipv6.begin(), attr.ipv6.end());
		
		for (auto& bytes : attr.ipv6) {
			be::big_to_native_inplace(bytes);
		}

		attr.ipv6[0] ^= MAGIC_COOKIE;
		attr.ipv6[1] ^= tx.tx_id[0];
		attr.ipv6[2] ^= tx.tx_id[1];
		attr.ipv6[3] ^= tx.tx_id[2];
	} else {
		logger_(Verbosity::STUN_LOG_DEBUG, LogReason::RESP_ADDR_FAM_NOT_VALID);
		throw std::exception("todo"); // todo
	}

	return attr;
}

std::future<std::expected<attributes::MappedAddress, LogReason>> Client::external_address() {
	std::promise<std::expected<attributes::MappedAddress, LogReason>> promise;
	auto future = promise.get_future();
	detail::Transaction::VariantPromise vp(std::move(promise));
	binding_request(std::move(vp));
	return future;
}

std::size_t Client::header_hash(const Header& header) {
	/*
	 * Hash the transaction ID to use as a key for future lookup.
	 * FNV is used because it's already in the project, not for any
	 * particular property. Odds of a collision are very low. 
	 */
	FNVHash fnv;

	if(mode_ == RFCMode::RFC5389) {
		fnv.update(header.tx_id_5389.begin(), header.tx_id_5389.end());
	} else {
		fnv.update(header.tx_id_3489.begin(), header.tx_id_3489.end());
	}

	return fnv.hash();
}

void Client::software() {

}

} // stun, ember