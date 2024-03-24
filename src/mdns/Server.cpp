 /*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Server.h"
#include "Socket.h"
#include "Parser.h"
#include "Utility.h"
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

void Server::handle_datagram(std::span<const std::uint8_t> datagram) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	// todo: temp
	std::cout << util::format_packet(datagram.data(), datagram.size()) << "\n";

	const auto result = parser::deserialise(datagram);
	if(!result) {
		LOG_WARN(logger_)
			<< "DNS query deserialising failed: "
			<< to_string(result.error())
			<< LOG_ASYNC;
		return;
	}

	LOG_TRACE(logger_) << to_string(result.value()) << LOG_ASYNC;

	if(result->header.flags.qr == 0) {
		handle_question(result.value());
	} else {
		handle_response(result.value());
	}
}

void Server::handle_question(const Query& query) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	LOG_WARN_GLOB << query.questions.size() << LOG_SYNC;

	for(const auto& query : query.questions) {
		LOG_WARN_GLOB << query.name << LOG_SYNC;
		LOG_WARN_GLOB << to_string(query.cc) << LOG_SYNC;
		LOG_WARN_GLOB << to_string(query.type) << LOG_SYNC;
	}

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

void Server::handle_response(const Query& query) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
}

void Server::shutdown() {
	// todo, broadcast withdrawal of services?
	socket_->close();
}

} // dns, ember