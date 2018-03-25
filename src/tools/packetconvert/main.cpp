/*
 * Copyright (c) 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "PacketLog_generated.h"
#include <game_protocol/Opcodes.h>
#include <shared/util/FormatPacket.h>
#include <boost/program_options.hpp>
#include <boost/endian/conversion.hpp>
#include <flatbuffers/flatbuffers.h>
#include <fstream>
#include <iostream>
#include <optional>
#include <thread>

namespace po = boost::program_options;

namespace ember {

void launch(const po::variables_map& args);
po::variables_map parse_arguments(int argc, const char* argv[]);

} // ember

int main(int argc, const char* argv[]) try {
	using namespace ember;

	const po::variables_map args = parse_arguments(argc, argv);
	launch(args);
} catch(std::exception& e) {
	std::cerr << e.what();
	return 1;
}

namespace ember {

enum class ReadState {
	SIZE, TYPE, BODY
};

template<typename T>
std::optional<T> try_read(std::ifstream& file) {
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

bool try_read(std::ifstream& file, std::vector<std::uint8_t>& buffer) {
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

void handle_header(const std::vector<std::uint8_t>& buff) {
	const auto header = flatbuffers::GetRoot<fblog::Header>(buff.data());
	flatbuffers::Verifier verifier(buff.data(), buff.size());

	if(!header->Verify(verifier)) {
		throw std::runtime_error("Flatbuffer verification failed");
	}

	std::cout << header->host()->str() << " " << header->host_desc() << " " << header->remote_host()->str() << "\n";
}

void handle_message(const std::vector<std::uint8_t>& buff) {
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

void handle_buffer(const fblog::Type type, const std::vector<std::uint8_t>& buff) {
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

void launch(const po::variables_map& args) {
	std::ifstream file(args.at("file").as<std::string>(),
		std::ifstream::in | std::ifstream::binary | std::ios::ate);

	const auto file_size = file.tellg();
	file.seekg(0);

	if(!file) {
		throw std::runtime_error("Unable to open packet dump file");
	}

	const auto interval = std::chrono::seconds(args.at("interval").as<unsigned int>());
	const auto stream = args.at("stream").as<bool>();
	auto skip = stream? args.at("skip").as<bool>() : false;

	ReadState state { ReadState::SIZE };
	std::optional<uint32_t> size;
	std::optional<fblog::Type> type;
	std::vector<std::uint8_t> buffer;

	while(stream || (!stream && file_size != file.tellg())) {
		if(state == ReadState::SIZE) {
			size = try_read<std::uint32_t>(file);

			if(!size) {
				std::this_thread::sleep_for(interval);
				skip = false;
				continue;
			}

			boost::endian::little_to_native_inplace(*size);
			state = ReadState::TYPE;
		}

		if(state == ReadState::TYPE) {
			type = try_read<fblog::Type>(file);

			if(!type) {
				std::this_thread::sleep_for(interval);
				skip = false;
				continue;
			}

			boost::endian::little_to_native_inplace(*type);
			state = ReadState::BODY;
		}

		if(state == ReadState::BODY) {
			buffer.resize(*size);

			auto ret = try_read(file, buffer);

			if(!ret) {
				std::this_thread::sleep_for(interval);
				skip = false;
				continue;
			}

			if(!skip) {
				handle_buffer(*type, buffer);
			}

			state = ReadState::SIZE;
		}
	}
}

po::variables_map parse_arguments(int argc, const char* argv[]) {
	po::options_description opt("Options");

	opt.add_options()
		("help,h", "Displays a list of available options")
		("file,f", po::value<std::string>()->default_value(""),
			"Path to packet capture dump file")
		("stream,s", po::bool_switch(),
			"Treat the input as a stream, monitoring for any new packets")
		("skip,k", po::bool_switch(),
			"If treating the packet dump as a stream, skip output of existing packets")
		("interval,i", po::value<unsigned int>()->default_value(2),
			"Frequency for checking the file stream for new packets")
		("format", po::value<std::string>(),
			"Todo")
		 ("filter", po::value<std::string>(),
		  "Todo");

	po::positional_options_description pos; 
	pos.add("file", 1);

	po::variables_map options;
	po::store(po::command_line_parser(argc, argv).positional(pos).options(opt)
	          .style(po::command_line_style::default_style & ~po::command_line_style::allow_guessing)
	          .run(), options);
	po::notify(options);

	if(options.count("help") || argc <= 1) {
		std::cout << opt << "\n";
		std::exit(0);
	}

	return options;
}

} // ember