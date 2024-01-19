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

using namespace std::chrono_literals;

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
	protocol_ = protocol;
	transport_.reset();

	switch(protocol) {
	case Protocol::UDP:
		transport_ = std::make_unique<DatagramTransport>(ctx_, host, port,
			[this](std::vector<std::uint8_t> buffer) { handle_response(std::move(buffer)); },
			[this](const boost::system::error_code& ec) { on_connection_error(ec); });
		break;
	case Protocol::TCP:
		transport_ = std::make_unique<StreamTransport>(ctx_, host, port,
			[this](std::vector<std::uint8_t> buffer) { handle_response(std::move(buffer)); },
			[this](const boost::system::error_code& ec) { on_connection_error(ec); });
		break;
	case Protocol::TLS_TCP:
		throw std::runtime_error("TLS_TCP STUN isn't supported");
	default:
		throw std::invalid_argument("Unknown protocol value specified");
	}

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
	auto timeout = protocol_ == UDP? udp_initial_timeout_ : tcp_timeout_;

	if(timeout == 0ms) {
		timeout = protocol_ == UDP? detail::UDP_TX_TIMEOUT : detail::TCP_TX_TIMEOUT;
	}

	auto max_retries = max_udp_retries_? max_udp_retries_ : detail::MAX_UDP_RETRIES;

	detail::Transaction tx(ctx_, timeout, max_retries);
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

	Header header{};
	stream >> header.type;
	stream >> header.length;

	if(mode_ == RFCMode::RFC5389) {
		stream >> header.cookie;
		stream.get(header.tx_id.id_5389.begin(), header.tx_id.id_5389.end());
	} else {
		stream.get(header.tx_id.id_3489.begin(), header.tx_id.id_3489.end());
	}

	// Check to see whether this is a response that we're expecting
	const auto hash = tx_hash(header.tx_id);

	if(!transactions_.contains(hash)) {
		logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_TX_NOT_FOUND);
		return;
	}

	detail::Transaction& transaction = transactions_.at(hash);
	transaction.timer.cancel();

	if(auto error = validate_header(header); error != Error::OK) {
		abort_transaction(transaction, error);
		return;
	}

	MessageType type{ static_cast<std::uint16_t>(header.type) };
	process_transaction(stream, transaction, type);
} catch(const spark::exception& e) {
	logger_(Verbosity::STUN_LOG_DEBUG, Error::BUFFER_PARSE_ERROR);
}

Error Client::validate_header(const Header& header) {
	if(mode_ == RFCMode::RFC5389 && header.cookie != MAGIC_COOKIE) {
		return Error::RESP_COOKIE_MISSING;
	}

	if(header.length < ATTR_HEADER_LENGTH) {
		return Error::RESP_BAD_HEADER_LENGTH;
	}

	MessageType type{ static_cast<std::uint16_t>(header.type) };

	if(type != MessageType::BINDING_RESPONSE &&
		type != MessageType::BINDING_ERROR_RESPONSE) {
		return Error::RESP_UNHANDLED_RESP_TYPE;
	}

	return Error::OK;
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
	auto attributes = handle_attributes(stream, tx, type);
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

template<typename T>
auto Client::extract_ipv4_pair(spark::BinaryInStream& stream) {
	stream.skip(1); // skip padding byte

 	T attr{};
	stream >> attr.family;
	stream >> attr.port;
	be::big_to_native_inplace(attr.port);
	stream >> attr.ipv4;
	be::big_to_native_inplace(attr.ipv4);

	if(attr.family != AddressFamily::IPV4) {
		throw Error::RESP_ADDR_FAM_NOT_VALID;
	}

	return attr;
}

template<typename T>
auto Client::extract_ip_pair(spark::BinaryInStream& stream) {
	stream.skip(1); // skip reserved byte
	T attr{};
	stream >> attr.family;
	stream >> attr.port;

	if(attr.family == AddressFamily::IPV4) {
		attr.family = AddressFamily::IPV4;
		be::big_to_native_inplace(attr.port);
		stream >> attr.ipv4;
		be::big_to_native_inplace(attr.ipv4);
	} else if(attr.family == AddressFamily::IPV6) {
		if(mode_ == RFCMode::RFC3489) {
			throw Error::RESP_IPV6_NOT_VALID;
		}

		stream.get(attr.ipv6.begin(), attr.ipv6.end());
		
		for(auto& bytes : attr.ipv6) {
			be::big_to_native_inplace(bytes);
		}
	} else {
		throw Error::RESP_ADDR_FAM_NOT_VALID;
	}
	
	return attr;
}

std::optional<attributes::Attribute> Client::extract_attribute(spark::BinaryInStream& stream,
                                                               const detail::Transaction& tx,
                                                               const MessageType type) {
	Attributes attr_type;
	be::big_uint16_t length;
	stream >> attr_type;
	stream >> length;
	be::big_to_native_inplace(attr_type);

	attributes::Attribute attribute;
	const bool required = (std::to_underlying(attr_type) >> 15) ^ 1;
	const bool attr_valid = check_attr_validity(attr_type, type, required);

	if(!attr_valid) {
		throw Error::RESP_UNEXPECTED_ATTR;
	}

	switch(attr_type) {
		case Attributes::MAPPED_ADDRESS:
			return extract_ip_pair<attributes::MappedAddress>(stream);
		case Attributes::XOR_MAPPED_ADDR_OPT: // it's a faaaaake!
			[[fallthrough]];
		case Attributes::XOR_MAPPED_ADDRESS:
			return parse_xor_mapped_address(stream, tx);
		case Attributes::CHANGED_ADDRESS:
			return extract_ipv4_pair<attributes::ChangedAddress>(stream);
		case Attributes::SOURCE_ADDRESS:
			return extract_ipv4_pair<attributes::SourceAddress>(stream);
		case Attributes::OTHER_ADDRESS:
			return extract_ip_pair<attributes::OtherAddress>(stream);
		case Attributes::RESPONSE_ORIGIN:
			return extract_ip_pair<attributes::ResponseOrigin>(stream);
		case Attributes::REFLECTED_FROM:
			return extract_ipv4_pair<attributes::ReflectedFrom>(stream);
		case Attributes::RESPONSE_ADDRESS:
			return extract_ipv4_pair<attributes::ResponseAddress>(stream);
		case Attributes::MESSAGE_INTEGRITY:
			return parse_message_integrity(stream);
		case Attributes::MESSAGE_INTEGRITY_SHA256:
			return parse_message_integrity_sha256(stream, length);
		case Attributes::USERNAME:
			return parse_username(stream, length);
		case Attributes::SOFTWARE:
			return parse_software(stream, length);
		case Attributes::ALTERNATE_SERVER:
			return extract_ip_pair<attributes::AlternateServer>(stream);
		case Attributes::FINGERPRINT:
			return parse_fingerprint(stream);
		case Attributes::ERROR_CODE:
			return parse_error_code(stream, length);
		case Attributes::UNKNOWN_ATTRIBUTES:
			return parse_unknown_attributes(stream, length);
	}

	logger_(Verbosity::STUN_LOG_DEBUG, required?
		Error::RESP_UNKNOWN_REQ_ATTRIBUTE : Error::RESP_UNKNOWN_OPT_ATTRIBUTE);

	// todo assert required
	// todo error handling

	stream.skip(length);
	return std::nullopt;
}

bool Client::check_attr_validity(const Attributes attr_type, const MessageType msg_type,
                                 const bool required) {
	/*
	 * If this attribute is marked as required, we'll look it up in the map
	 * to check whether we know what it is and more importantly whose fault
	 * it is if we can't finish parsing the message, given our current RFC mode
	 */
	if(required) {
		if(const auto rfc = attr_req_lut.find(attr_type); rfc != attr_req_lut.end()) {
			if (!(rfc->second & mode_)) { // definitely not our fault... probably
				logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_BAD_REQ_ATTR_SERVER);
				return false;
			}
		}
		else {
			// might be our fault but probably not
			logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_UNKNOWN_REQ_ATTRIBUTE);
			return false;
		}
	}

	// Check whether this attribute is valid for the given response type
	if(const auto entry = attr_valid_lut.find(attr_type); entry != attr_valid_lut.end()) {
		if(entry->second != msg_type) { // not valid for this type
			logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_BAD_REQ_ATTR_SERVER);
			return false;
		}
	} else { // not valid for *any* response type
		logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_BAD_REQ_ATTR_SERVER);
		return false;
	}

	return true;
}

std::vector<attributes::Attribute>
Client::handle_attributes(spark::BinaryInStream& stream, const detail::Transaction& tx, const MessageType type) {
	std::vector<attributes::Attribute> attributes;

	while(!stream.empty()) {
		auto attribute = extract_attribute(stream, tx, type);

		if(attribute) {
			attributes.emplace_back(std::move(*attribute));
		}
	}

	return attributes;
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

attributes::XorMappedAddress
Client::parse_xor_mapped_address(spark::BinaryInStream& stream, const detail::Transaction& tx) {
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
		attr.ipv6[1] ^= tx.tx_id.id_5389[0];
		attr.ipv6[2] ^= tx.tx_id.id_5389[1];
		attr.ipv6[3] ^= tx.tx_id.id_5389[2];
	} else {
		throw Error::RESP_ADDR_FAM_NOT_VALID;
	}

	return attr;
}

attributes::Fingerprint Client::parse_fingerprint(spark::BinaryInStream& stream) {
	attributes::Fingerprint attr{};
	stream >> attr.crc32;
	be::big_to_native_inplace(attr.crc32);
	return attr;
}

attributes::Software
Client::parse_software(spark::BinaryInStream& stream, const std::size_t size) {
	// UTF8 encoded sequence of less than 128 characters (which can be as long as 763 bytes)
	if(size > 763) {
		logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_BAD_SOFTWARE_ATTR);
	}

	attributes::Software attr{};
	attr.description.resize(size);
	stream.get(attr.description.begin(), attr.description.end());
	return attr;
}

attributes::MessageIntegrity
Client::parse_message_integrity(spark::BinaryInStream& stream) {
	attributes::MessageIntegrity attr{};
	stream.get(attr.hmac_sha1.begin(), attr.hmac_sha1.end());
	return attr;
}

attributes::MessageIntegrity256
Client::parse_message_integrity_sha256(spark::BinaryInStream& stream, const std::size_t length) {
	attributes::MessageIntegrity256 attr{};

	if (length < 16 || length > attr.hmac_sha256.size()) {
		throw Error::RESP_BAD_HMAC_SHA_ATTR;
	}

	stream.get(attr.hmac_sha256.begin(), attr.hmac_sha256.begin() + length);
	return attr;
}

attributes::Username
Client::parse_username(spark::BinaryInStream& stream, const std::size_t size) {
	attributes::Username attr{};
	attr.username.resize(size);
	stream.get(attr.username.begin(), attr.username.end());
	return attr;
}

attributes::ErrorCode
Client::parse_error_code(spark::BinaryInStream& stream, std::size_t length) {
	attributes::ErrorCode attr{};
	stream >> attr.code;

	if(attr.code & 0xFFE00000) {
		logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_ERROR_CODE_OUT_OF_RANGE);
	}

	// (╯°□°）╯︵ ┻━┻
	const auto code = (attr.code >> 8) & 0x07;
	const auto num = attr.code & 0xFF;

	if(code < 300 || code >= 700) {
		if(mode_ == RFCMode::RFC5389) {
			logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_ERROR_CODE_OUT_OF_RANGE);
		} else if(code < 100) { // original RFC has a wider range (1xx - 6xx)
			logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_ERROR_CODE_OUT_OF_RANGE);
		}
	}

	if(num >= 100) {
		logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_ERROR_CODE_OUT_OF_RANGE);
	}

	attr.code = (code * 100) + num;

	std::string reason;
	reason.resize(length - sizeof(attributes::ErrorCode::code));
	stream.get(reason.begin(), reason.end());

	if(reason.size() % 4) {
		logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_ERROR_STRING_BAD_PAD);
	}

	return attr;
}

attributes::UnknownAttributes
Client::parse_unknown_attributes(spark::BinaryInStream& stream, std::size_t length) {
	if(length % 2) {
		throw Error::RESP_UNK_ATTR_BAD_PAD;
	}
	
	attributes::UnknownAttributes attr{};

	while(length) {
		Attributes attr_type;
		stream >> attr_type;
		be::big_to_native_inplace(attr_type);
		attr.attributes.emplace_back(attr_type);
	}

	if(attr.attributes.size() % 2) {
		logger_(Verbosity::STUN_LOG_DEBUG, Error::RESP_UNK_ATTR_BAD_PAD);
	}

	return attr;
}

void Client::set_udp_timer(detail::Transaction& tx) {
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
					tx.timeout = tx.initial_to * detail::TX_RM;
				}

				binding_request(tx);
			} else {
				abort_transaction(tx, Error::NO_RESPONSE_RECEIVED);
			}
		}
	});
}

void Client::set_tcp_timer(detail::Transaction& tx) {
	tx.timer.expires_from_now(tx.timeout);
	tx.timer.async_wait([&, hash = tx.hash](const boost::system::error_code& ec) {
		if(ec) {
			return;
		}

		if(auto entry = transactions_.find(hash); entry != transactions_.end()) {
			abort_transaction(tx, Error::NO_RESPONSE_RECEIVED);
		}
	});
}

void Client::transaction_timer(detail::Transaction& tx) {
	if(protocol_ != Protocol::UDP) {
		set_tcp_timer(tx);
	} else {
		set_udp_timer(tx);
	}
}

void Client::set_udp_initial_timeout(std::chrono::milliseconds timeout) {
	if(timeout <= 0ms) {
		throw std::invalid_argument("Timeout must be > 0ms");
	}

	udp_initial_timeout_ = timeout;
}

void Client::set_max_udp_retries(int retries) {
	if(retries < 0) {
		throw std::invalid_argument("UDP retries cannot be negative");
	}

	max_udp_retries_ = retries;
}

void Client::set_tcp_timeout(std::chrono::milliseconds timeout) {
	if(timeout <= 0ms) {
		throw std::invalid_argument("Timeout must be > 0ms");
	}

	tcp_timeout_ = timeout;
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