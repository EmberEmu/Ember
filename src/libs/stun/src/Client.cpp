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
#include <boost/asio.hpp>
#include <boost/endian.hpp>
#include <stdexcept>
#include <utility>
#include <vector>
#include <cstddef>

#include <iostream> // todo temp

namespace ember::stun {

Client::Client(RFCMode mode) : mode_(mode) {
	work_.emplace_back(std::make_shared<boost::asio::io_context::work>(ctx_));
	worker_ = std::jthread(static_cast<size_t(boost::asio::io_context::*)()>
		(&boost::asio::io_context::run), &ctx_);
}

Client::~Client() {
	work_.clear();
	transport_.reset();
	ctx_.stop();
}

void Client::connect(const std::string& host, const std::uint16_t port, const Protocol protocol) {
	transport_.reset();

	switch(protocol) {
	case Protocol::UDP:
		transport_ = std::make_unique<DatagramTransport>(ctx_, host, port,
			[this](std::vector<std::uint8_t> buffer) { handle_response(std::move(buffer)); });
		break;
	case Protocol::TCP:
		transport_ = std::make_unique<StreamTransport>(host, port);
		break;
	case Protocol::TLS_TCP:
		throw std::runtime_error("TLS_TCP STUN isn't handled yet");
	}

	transport_->connect();
}

void Client::handle_response(std::vector<std::uint8_t> buffer) try {
	if(buffer.size() < HEADER_LENGTH) {
		return; // just ignore this message
	}

	spark::VectorBufferAdaptor<std::uint8_t> vba(buffer);
	spark::BinaryInStream stream(vba);

	Header header{};
	stream >> header.type;
	stream >> header.length;
	stream >> header.cookie;
	stream.get(header.trans_id_5389, sizeof(header.trans_id_5389));

	if(mode_ == RFCMode::RFC5389 && header.cookie != MAGIC_COOKIE) {
		// todo
	}

	if(header.length < ATTR_HEADER_LENGTH) {
		// todo
	}

	handle_attributes(stream);
} catch(const std::exception& e) {
	std::cout << e.what(); // temp
}

void Client::handle_attributes(spark::BinaryInStream& stream) {
	Attributes attribute;
	be::big_uint16_t length;
	stream >> attribute;
	stream >> length;

	be::big_to_native_inplace(attribute);

	switch(attribute) {
		case Attributes::MAPPED_ADDRESS:
			handle_mapped_address(stream, length);
			break;
		case Attributes::XOR_MAPPED_ADDRESS:
			handle_xor_mapped_address(stream, length);
			break;
	}
}

void Client::xor_buffer(std::span<std::uint8_t> buffer, const std::vector<std::uint8_t>& key) {
	for (std::size_t i = 0u; i < buffer.size();) {
		for (std::size_t j = 0u; j < key.size(); ++j) {
			buffer[i] ^= key.data()[j];
			++i;
		}
	}
}

void Client::handle_mapped_address(spark::BinaryInStream& stream, const std::uint16_t length) {
	if (mode_ == RFCMode::RFC5389) {
		// todo, warning
	}

	stream.skip(1); // skip reserved byte
	AddressFamily addr_fam = AddressFamily::IPV4;
	stream >> addr_fam;

	if(addr_fam != AddressFamily::IPV4) {
		// todo, error
	}

	std::uint16_t port = 0;
	stream >> port;
	be::big_to_native_inplace(port);

	std::uint32_t ipv4 = 0;
	stream >> ipv4;
	be::big_to_native_inplace(ipv4);

	boost::asio::ip::address_v4 addr(ipv4);
	std::cout << addr.to_string() << ":" << port;
}

void Client::handle_xor_mapped_address(spark::BinaryInStream& stream, const std::uint16_t length) {
	if(mode_ == RFCMode::RFC3489) {
		// todo, warning
	}

	stream.skip(1); // skip reserved byte
	AddressFamily addr_fam = AddressFamily::IPV4;
	stream >> addr_fam;

	// XOR port with the magic cookie
	std::uint16_t port = 0;
	stream >> port;
	auto magic = MAGIC_COOKIE;
	be::native_to_big_inplace(magic);
	port ^= magic;
	be::big_to_native_inplace(port);

	if(addr_fam == AddressFamily::IPV4) {
		std::uint32_t ipv4 = 0;
		stream >> ipv4;
		be::big_to_native_inplace(ipv4);
		ipv4 ^= MAGIC_COOKIE;
		boost::asio::ip::address_v4 addr(ipv4);
		std::cout << addr.to_string();
	} else if(addr_fam == AddressFamily::IPV6) {
		/*
		spark::VectorBufferAdaptor<std::uint8_t> vba(xor_data);
		spark::BinaryInStream xor_stream(vba);
		std::uint64_t ipv6 = 0;
		xor_stream >> ipv6;
		be::big_to_native_inplace(ipv6);*/
	} else {
		// todo
	}
}

std::future<std::string> Client::mapped_address() {
	std::vector<std::uint8_t> data;
	spark::VectorBufferAdaptor buffer(data);
	spark::BinaryOutStream stream(buffer);

	Header header { };
	header.type = (uint16_t)Attributes::MAPPED_ADDRESS;
	header.length = 0;

	if(mode_ == RFCMode::RFC5389) {
		header.trans_id_5389[0] = 5;
		header.cookie = MAGIC_COOKIE;
	} else {
		header.trans_id_3489[0] = 5;
	}

	stream << header.type;
	stream << header.length;

	if(mode_ == RFCMode::RFC5389) {
		stream << header.cookie;
		stream << header.trans_id_5389;
	} else {
		stream << header.trans_id_3489;
	}

	transport_->send(data);
	return result.get_future();
}

void Client::software() {
	std::vector<std::uint8_t> data;
	spark::VectorBufferAdaptor buffer(data);
	spark::BinaryOutStream stream(buffer);

	Header header{ };
	header.type = (uint16_t)Attributes::SOFTWARE;
	header.length = 0;
	header.trans_id_5389[0] = 5;

	if (mode_ == RFCMode::RFC5389) {
		header.cookie = MAGIC_COOKIE;
	}
	else {
		header.cookie = 0;
	}

	stream << header;
	transport_->send(data);
}

} // stun, ember