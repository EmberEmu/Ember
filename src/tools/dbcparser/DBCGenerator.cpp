﻿/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "DBCGenerator.h"
#include "TypeUtils.h"
#include <spark/buffers/BinaryStream.h>
#include <spark/buffers/ChainedBuffer.h>
#include <logger/Logging.h>
#include <boost/endian/arithmetic.hpp>
#include <fstream>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace be = boost::endian;

namespace ember::dbc {

class RecordPrinter : public types::TypeVisitor {
	ember::spark::ChainedBuffer<4096> buffer_, sb_buffer_;
	ember::spark::BinaryStream record_, string_block_;
	const std::string default_string = "Hello, world!";

	void generate_field(const std::string& type) {
		if(type == "int8" || type == "uint8" || type == "bool") {
			record_ << std::uint8_t(1);
		} else if(type == "int16" || type == "uint16") {
			record_ << std::uint16_t(1);
		} else if(type == "int32" || type == "uint32" || type == "bool32") {
			record_ << std::uint32_t(1);
		} else if(type == "float") {
			record_ << 1.0f;
		} else if(type == "double") {
			record_ << 1.0;
		} else if(type == "string_ref") {
			record_ << std::uint32_t(string_block_.size());
			string_block_ << default_string;
		} else if(type == "string_ref_loc") {
			for(int i = 0; i < 9; ++i) {
				record_ << std::uint32_t(string_block_.size());
				string_block_ << default_string;
			}
		}
	}

public:
	RecordPrinter() : record_(buffer_), string_block_(sb_buffer_) {
		// DBC editors choke if the block doesn't start with 0 with the first string at offset 1
		// This is apparently just DBC editors being bad, which is why this tool exists to begin with
		string_block_ << std::uint8_t(0);
	}

	void visit(const types::Struct* type) override {
		// we don't care about structs
	}

	void visit(const types::Enum* type) override {
		generate_field(type->underlying_type);
	}

	void visit(const types::Field* type) override {
		auto components = extract_components(type->underlying_type);
		std::size_t elements = 1;

		// if this is an array, we need to write multiple records
		if(components.second) {
			elements = *components.second;
		}

		for(std::size_t i = 0; i < elements; ++i) {
			generate_field(components.first);
		}
	}

	std::vector<char> string_block() {
		std::vector<char> data(string_block_.size());
		string_block_.get(data.data(), data.size());
		return data;
	}

	std::vector<char> record() {
		std::vector<char> data(record_.size());
		record_.get(data.data(), data.size());
		return data;
	}
};

void generate_template(const types::Struct* dbc) {
	LOG_DEBUG_GLOB << "Generating template for " << dbc->name << LOG_ASYNC;

	std::ofstream file(dbc->name + ".dbc", std::ofstream::binary);
	
	TypeMetrics metrics;
	walk_dbc_fields(dbc, metrics);

	RecordPrinter printer;
	walk_dbc_fields(dbc, printer);

	std::vector<char> record_data = printer.record();
	std::vector<char> string_data = printer.string_block();

	be::big_uint32_t magic('WDBC');
	be::little_uint32_t records = 1;
	be::little_uint32_t fields = metrics.fields;
	be::little_uint32_t record_size = metrics.record_size;
	be::little_uint32_t string_block_size = string_data.size();
	
	// write header
	file.write((char*)&magic, sizeof(magic));
	file.write((char*)&records, sizeof(records));
	file.write((char*)&fields, sizeof(fields));
	file.write((char*)&record_size, sizeof(record_size));
	file.write((char*)&string_block_size, sizeof(string_block_size));
	
	// write dummy record
	file.write(record_data.data(), record_data.size());

	// write string block
	file.write(string_data.data(), string_data.size());

	LOG_DEBUG_GLOB << "Completed template generation for " << dbc->name << LOG_ASYNC;
}

} // dbc, ember