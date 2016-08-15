/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "DBCGenerator.h"
#include "TypeUtils.h"
#include <spark/BinaryStream.h>
#include <spark/buffers/ChainedBuffer.h>
#include <boost/endian/arithmetic.hpp>
#include <fstream>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace be = boost::endian;

namespace ember { namespace dbc {

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
		int scalar_size = type_size_map.at(components.first);
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

class TypeMetrics : public types::TypeVisitor {
public:
	int fields = 0;
	int size = 0;

	void visit(const types::Struct* type) override {
		// we don't care about structs
	}

	void visit(const types::Enum* type) override {
		++fields;
		size += type_size_map.at(type->underlying_type);
	}

	void visit(const types::Field* type) override {
		auto components = extract_components(type->underlying_type);
		int scalar_size = type_size_map.at(components.first);
		
		// handle arrays
		if(components.second) {
			fields += *components.second;
			size += (scalar_size * (*components.second));
		} else {
			++fields;
			size += scalar_size;
		}
	}
};

template<typename T>
void walk_dbc_fields(const types::Struct* dbc, T& visitor) {
	for(auto f : dbc->fields) {
		std::string type = f.underlying_type;

		// if this is a user-defined struct, we need to go through that type too
		// if it's an enum, we can just grab the underlying type
		auto components = extract_components(f.underlying_type);
		auto it = type_map.find(components.first);

		if(it != type_map.end()) {
			visitor.visit(&f);
		} else {
			auto found = locate_type_base(*dbc, f.underlying_type);

			if(!found) {
				throw std::runtime_error("Unknown field type encountered, " + f.underlying_type);
			}

			if(found->type == types::STRUCT) {
				walk_dbc_fields(static_cast<types::Struct*>(found), visitor);
			} else if(found->type == types::ENUM) {
				visitor.visit(static_cast<types::Enum*>(found));
			}
		}
	}
}

void generate_template(const types::Struct* dbc) {
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
	be::little_uint32_t size = metrics.size;
	be::little_uint32_t string_block_size = string_data.size();
	
	// write header
	file.write((char*)&magic, sizeof(magic));
	file.write((char*)&records, sizeof(records));
	file.write((char*)&fields, sizeof(fields));
	file.write((char*)&size, sizeof(size));
	file.write((char*)&string_block_size, sizeof(string_block_size));
	
	// write dummy record
	file.write(record_data.data(), record_data.size());

	// write string block
	file.write(string_data.data(), string_data.size());
}

}} // dbc, ember