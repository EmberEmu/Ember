 /*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Server.h"
#include "Socket.h"
#include "Parser.h"
#include <shared/util/FormatPacket.h>
#include <iostream>

namespace ember::dns {

Server::Server(std::unique_ptr<Socket> socket, log::Logger* logger)
               : socket_(std::move(socket)), logger_(logger) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
    socket_->register_handler(this);
}

Server::~Server() {
	shutdown();
}

void Server::shutdown() {
	if(!socket_) {
		return;
	}

	// todo, broadcast withdrawal of services?
	socket_->deregister_handler(this);
	socket_.reset();
}

void Server::handle_query(std::span<const std::byte> datagram) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	//Query out{};
	//out.questions = query.questions;
	//out.header.questions = query.header.questions;
	//out.header.id = query.header.id;
	//out.header.flags.opcode = Opcode::STANDARD_QUERY;
	//out.header.flags.reply_code = ReplyCode::REPLY_NO_ERROR;
	//out.header.flags.response = 1;

	//if (query.header.answers || query.header.additional || query.header.authorities) {
	//	out.header.flags.reply_code = ReplyCode::FORMAT_ERROR;
	//	return out;
	//}

	//if (!query.header.questions) {
	//	return out;
	//}

	//for (auto& question : query.questions) {
	//	auto it = records_.find({ question.type, question.name });

	//	if (it == records_.end()) {
	//		continue;
	//	}

	//	ResourceRecord record{};
	//	record.name = it->second.name;
	//	record.type = it->second.type;

	//	if (record.type == RecordType::A && it->second.answer.is_v4()) {
	//		Record_A rdata{ it->second.answer.to_v4().to_ulong() };
	//		record.rdata = rdata;
	//	}
	//	else if (record.type == RecordType::AAAA && it->second.answer.is_v6()) {
	//		Record_AAAA rdata{ it->second.answer.to_v6().to_bytes() };
	//		record.rdata = rdata;
	//	}
	//	else {
	//		out.header.flags.reply_code = ReplyCode::SERVER_FAILURE;
	//		break;
	//	}

	//	record.ttl = it->second.ttl;
	//	record.resource_class = Class::CLASS_IN;
	//	out.answers.emplace_back(std::move(record));
	//}

	//out.header.answers = gsl::narrow<std::uint16_t>(out.answers.size());
	//return out;
}

void Server::handle_response(std::span<const std::byte> datagram) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
}

void Server::handle_datagram(std::span<const std::byte> datagram) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	const auto& result = Parser::validate(datagram);

    if(result != Result::OK) {
        LOG_DEBUG(logger_) << "DNS validation failed, " << to_string(result) << LOG_ASYNC;
        return;
    }

	// todo
	std::cout << util::format_packet((unsigned char*)datagram.data(), datagram.size()) << "\n";

    const auto header = Parser::header_overlay(datagram);
    const auto flags = Parser::decode_flags(header->flags);
    
	// temp
	LOG_DEBUG(logger_) << "ID: " << header->id << LOG_ASYNC;
	LOG_DEBUG(logger_) << "Questions: " << header->questions << LOG_ASYNC;

    if(flags.qr == 0) {
        handle_query(datagram);
    } else {
        handle_response(datagram);
    }
}

//void print_record(log::Logger& logger, const ResourceRecord& record) {
//	logger << record.name;
//	logger << record.rdata_len;
//	logger << to_string(record.resource_class);
//	logger << record.ttl;
//	logger << to_string(record.type) << "\n";
//}
//
//log::Logger& operator<<(log::Logger& logger, const Query& query) {
//	logger << "Query information: \n";
//	logger << "Sequence: " << query.header.id << "\n";
//	logger << "Flags: " << to_string(query.header.flags.opcode) << "\n";
//
//	logger << "[questions]\n";
//
//	for (auto& question : query.questions) {
//		logger << question.name << ", " << to_string(question.type)
//			<< ", " << to_string(question.rclass) << "\n";
//	}
//
//	logger << "[answers]\n";
//
//	for (auto& answer : query.answers) {
//		print_record(logger, answer);
//	}
//
//	logger << "[authorities]\n";
//
//	for (auto& answer : query.authorities) {
//		print_record(logger, answer);
//	}
//
//	logger << "[additional]\n";
//
//	for (auto& answer : query.additional) {
//		print_record(logger, answer);
//	}
//
//	return logger;
//}


} // dns, ember