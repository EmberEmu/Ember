/*
 * Copyright (c) 2016 - 2019 Ember
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
#include <gsl/gsl_util>
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
	const std::string default_string_loc = "Hello, localised string!";

	void generate_field(const std::string& type) {
		if(type == "int8" || type == "uint8" || type == "bool") {
			record_ << std::uint8_t{1};
		} else if(type == "int16" || type == "uint16") {
			record_ << std::uint16_t{1};
		} else if(type == "int32" || type == "uint32" || type == "bool32") {
			record_ << std::uint32_t{1};
		} else if(type == "float") {
			record_ << 1.0f;
		} else if(type == "double") {
			record_ << 1.0;
		} else if(type == "string_ref") {
			record_ << gsl::narrow<std::uint32_t>(string_block_.size());
			string_block_ << default_string;
		} else if(type == "string_ref_loc") {
			for(const auto& loc : string_ref_loc_regions) {
				record_ << gsl::narrow<std::uint32_t>(string_block_.size());
				string_block_ << default_string_loc;
			}

			record_ << std::uint32_t{1};
		} else {
			throw std::runtime_error("Encountered an unhandled type, " + type);
		}
	}

public:
	RecordPrinter() : record_(buffer_), string_block_(sb_buffer_) {
		// DBC editors choke if the block doesn't start with 0 with the first string at offset 1
		// This is apparently just DBC editors being bad, which is why this tool exists to begin with
		string_block_ << std::uint8_t{0};
	}

	void visit(const types::Struct* type, const types::Field* parent) override {
		walk_dbc_fields(*this, type, parent);
	}

	void visit(const types::Enum* type) override {
		generate_field(type->underlying_type);
	}

	void visit(const types::Field* field, const types::Base* parent) override {
		auto components = extract_components(field->underlying_type);
		std::size_t elements = 1;

		// if this is an array, we need to write multiple records
		if(components.second) {
			elements = *components.second;
		}

		if(type_map.find(components.first) == type_map.end()) {
			const auto found = locate_type_base(static_cast<types::Struct&>(*field->parent), components.first);

			if(!found) {
				throw std::runtime_error("Unable to locate type base");
			}

			if(found->type == types::STRUCT) {				
				visit(static_cast<const types::Struct*>(found), field);
				return;
			} else if(found->type == types::ENUM) {
				const auto type = static_cast<const types::Enum*>(found);
				components.first = type->underlying_type;
			} else {
				throw std::runtime_error("Unhandled type");
			}
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

void generate_dbc_template(const types::Struct* dbc, const std::string& out_path) {
	LOG_INFO_GLOB << "Generating template for " << dbc->name << LOG_ASYNC;

	std::ofstream file(out_path + dbc->name + ".dbc", std::ofstream::binary);
	
	if(!file) {
		throw std::runtime_error("Unable to open DBC for template generation");
	}

	TypeMetrics metrics;
	walk_dbc_fields(metrics, dbc, dbc->parent);

	RecordPrinter printer;
	walk_dbc_fields(printer, dbc, dbc->parent);

	const std::vector<char> record_data = printer.record();
	const std::vector<char> string_data = printer.string_block();

	const be::big_uint32_t magic('WDBC');
	const be::little_uint32_t records = 1;
	const be::little_uint32_t fields = metrics.fields;
	const be::little_uint32_t record_size = metrics.record_size;
	const be::little_uint32_t string_block_size = string_data.size();
	
	// write header
	file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
	file.write(reinterpret_cast<const char*>(&records), sizeof(records));
	file.write(reinterpret_cast<const char*>(&fields), sizeof(fields));
	file.write(reinterpret_cast<const char*>(&record_size), sizeof(record_size));
	file.write(reinterpret_cast<const char*>(&string_block_size), sizeof(string_block_size));
	
	// write dummy record
	file.write(record_data.data(), record_data.size());

	// write string block
	file.write(string_data.data(), string_data.size());

	if(!file) {
		throw std::runtime_error("An error occured during DBC template generation");
	}

	LOG_DEBUG_GLOB << "Completed template generation for " << dbc->name << LOG_ASYNC;
}

} // dbc, ember