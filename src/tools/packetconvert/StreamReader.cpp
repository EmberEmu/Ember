/*
 * Copyright (c) 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "StreamReader.h"
#include <boost/endian/conversion.hpp>
#include <game_protocol/Opcodes.h>
#include <shared/util/FormatPacket.h>
#include <optional>
#include <thread>

namespace ember {

StreamReader::StreamReader(std::ifstream& in, bool stream, bool skip, std::chrono::seconds interval)
                           : in_(in), skip_(skip), stream_(stream), interval_(interval),
                             stream_size_(0) {
	in.seekg(0, std::ifstream::end);
	stream_size_ = in.tellg();
	in.seekg(0);

	if(!in || in.flags() & in.binary) {
		throw std::runtime_error("Packet dump stream error");
	}
}

void StreamReader::process() {
	ReadState state { ReadState::SIZE };
	std::optional<uint32_t> size;
	std::optional<fblog::Type> type;
	std::vector<std::uint8_t> buffer;

	while(stream_ || (!stream_ && stream_size_ != in_.tellg())) {
		if(state == ReadState::SIZE) {
			size = try_read<std::uint32_t>(in_);

			if(!size) {
				std::this_thread::sleep_for(interval_);
				skip_ = false;
				continue;
			}

			boost::endian::little_to_native_inplace(*size);
			state = ReadState::TYPE;
		}

		if(state == ReadState::TYPE) {
			type = try_read<fblog::Type>(in_);

			if(!type) {
				std::this_thread::sleep_for(interval_);
				skip_ = false;
				continue;
			}

			boost::endian::little_to_native_inplace(*type);
			state = ReadState::BODY;
		}

		if(state == ReadState::BODY) {
			buffer.resize(*size);

			auto ret = try_read(in_, buffer);

			if(!ret) {
				std::this_thread::sleep_for(interval_);
				skip_ = false;
				continue;
			}

			if(!skip_) {
				handle_buffer(*type, buffer);
			}

			state = ReadState::SIZE;
		}
	}
}

template<typename T>
std::optional<T> StreamReader::try_read(std::ifstream& file) {
	static_assert(std::is_trivially_copyable<T>::value, "Cannot safely read this type");

	T val;

	const auto pos = file.tellg();
	file.read(reinterpret_cast<char*>(&val), sizeof(val));

	if(file.good()) {
		return val;
	} else {
		file.clear();
		file.seekg(pos);
		return std::nullopt;
	}
}

bool StreamReader::try_read(std::ifstream& file, std::vector<std::uint8_t>& buffer) {
	const auto pos = file.tellg();

	file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

	if(file.good()) {
		return true;
	} else {
		file.clear();
		file.seekg(pos);
		return false;
	}
}

void StreamReader::handle_header(const std::vector<std::uint8_t>& buff) {
	const auto header = flatbuffers::GetRoot<fblog::Header>(buff.data());
	flatbuffers::Verifier verifier(buff.data(), buff.size());

	if(!header->Verify(verifier)) {
		throw std::runtime_error("Flatbuffer verification failed");
	}

	std::cout << header->host()->str() << " " << header->host_desc() << " " << header->remote_host()->str() << "\n";
}

void StreamReader::handle_message(const std::vector<std::uint8_t>& buff) {
	auto message = flatbuffers::GetRoot<fblog::Message>(buff.data());
	flatbuffers::Verifier verifier(buff.data(), buff.size());

	if(!message->Verify(verifier)) {
		throw std::runtime_error("Flatbuffer verification failed");
	}

	protocol::ClientOpcode c_op;
	protocol::ServerOpcode s_op;
	std::string op_desc;

	switch(message->direction()) {
		case fblog::Direction::INBOUND:
			std::memcpy(&c_op, message->payload()->data(), sizeof(c_op));
			op_desc = protocol::to_string(c_op);
			break;
		case fblog::Direction::OUTBOUND:
			std::memcpy(&s_op, message->payload()->data(), sizeof(s_op));
			op_desc = protocol::to_string(s_op);
			break;
		default:
			throw std::runtime_error("Unknown message direction");
	}

	const auto payload = message->payload();
	std::cout << message->time()->str() << " " << "\n";
	std::cout << op_desc << "\n" << util::format_packet(payload->data(), payload->size()) << std::endl;
}

void StreamReader::handle_buffer(const fblog::Type type, const std::vector<std::uint8_t>& buff) {
	switch(type) {
		case fblog::Type::HEADER:
			handle_header(buff);
			break;
		case fblog::Type::MESSAGE:
			handle_message(buff);
			break;
		default:
			throw std::runtime_error("Unknown message type");
	}
}

} // ember