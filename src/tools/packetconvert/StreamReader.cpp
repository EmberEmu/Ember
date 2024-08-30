/*
 * Copyright (c) 2018 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "StreamReader.h"
#include <boost/endian/conversion.hpp>
#include <boost/container/small_vector.hpp>
#include <thread>

namespace ember {

StreamReader::StreamReader(std::ifstream& in, std::uintmax_t size, bool stream, bool skip,
                           std::chrono::seconds interval)
                           : in_(in), skip_(skip), stream_(stream), interval_(interval),
                             stream_size_(size) {
	if(!in) {
		throw std::runtime_error("Packet dump stream error");
	}
}

void StreamReader::add_sink(std::unique_ptr<Sink> sink) {
	sinks_.emplace_back(std::move(sink));
}

void StreamReader::process() {
	ReadState state { ReadState::SIZE };
	std::optional<fblog::Type> type;
	boost::container::small_vector<std::uint8_t, 256> buffer;

	while(stream_ || stream_size_ != in_.tellg()) {
		if(state == ReadState::SIZE) {
			auto size = try_read<std::uint32_t>(in_);

			if(!size) {
				std::this_thread::sleep_for(interval_);
				skip_ = false;
				continue;
			}

			boost::endian::little_to_native_inplace(*size);
			buffer.resize(*size, boost::container::default_init);
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

bool StreamReader::try_read(std::ifstream& file, std::span<std::uint8_t> buffer) {
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

void StreamReader::handle_header(std::span<const std::uint8_t> buff) {
	const auto header = flatbuffers::GetRoot<fblog::Header>(buff.data());
	flatbuffers::Verifier verifier(buff.data(), buff.size());

	if(!header->Verify(verifier)) {
		throw std::runtime_error("Flatbuffer verification failed");
	}

	for(auto& sink : sinks_) {
		sink->handle(*header);
	}
}

void StreamReader::handle_message(std::span<const std::uint8_t> buff) {
	auto message = flatbuffers::GetRoot<fblog::Message>(buff.data());
	flatbuffers::Verifier verifier(buff.data(), buff.size());

	if(!message->Verify(verifier)) {
		throw std::runtime_error("Flatbuffer verification failed");
	}

	for(auto& sink : sinks_) {
		sink->handle(*message);
	}
}

void StreamReader::handle_buffer(const fblog::Type type, std::span<const std::uint8_t> buff) {
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