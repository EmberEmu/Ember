/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Utility.h"
#include "DNSDefines.h"
#include <format>
#include <sstream>

namespace ember::dns {

std::string to_string(const ResourceRecord& record) {
	std::stringstream stream;

	if(record.type == RecordType::opt) {
		// todo, should extend the parser to handle invalid OPT name values
		if(record.name.empty()) {
			stream << "Name: <root>" << '\n';
		} else {
			stream << "Name: " << "[invalid data]" << '\n';
		}

		stream << "Payload size: " << std::to_underlying(record.resource_class) << '\n';
		stream << "Ext. RCODE & flags: " << record.ttl << '\n';
	} else {
		stream << "Name: " << record.name << '\n';
		stream << "Class: " << record.resource_class << '\n';
		stream << "TTL: " << record.ttl << '\n';
	}

	stream << "Type: " << to_string(record.type) << '\n';
	stream << "Len: " << record.rdata_len << '\n';
	return stream.str();
}

std::string to_string(const Query& query) {
	std::stringstream stream;
	stream << "Query information: \n";
	stream << "Sequence: " << query.header.id << '\n';
	stream << "Flags: " << to_string(query.header.flags.opcode) << '\n';

	stream << std::format("[questions][{}]\n", query.questions.size());

	for(auto& question : query.questions) {
		stream << question.name << ", " << to_string(question.type)
			<< ", " << to_string(question.cc) << '\n';
	}

	stream << std::format("[answers][{}]\n", query.answers.size());

	for(auto& answer : query.answers) {
		stream << "[record]" << '\n';
		stream << to_string(answer) << '\n';
	}

	stream << std::format("[authorities][{}]\n", query.authorities.size());

	for(auto& answer : query.authorities) {
		stream << "[record]" << '\n';
		stream << to_string(answer) << '\n';
	}

	stream << std::format("[additional][{}]\n", query.additional.size());

	for(auto& answer : query.additional) {
		stream << "[record]" << '\n';
		stream << to_string(answer) << '\n';
	}

	return stream.str();
}

} // dns, ember