/*
 * Copyright (c) 2019 - 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "SQLDDLGenerator.h"
#include "TypeUtils.h"
#include <logger/Logging.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <cstddef>
#include <cstdint>

namespace ember::dbc {

class DDLPrinter final : public types::TypeVisitor {
	std::stringstream out_;
	std::vector<std::string> columns_;
	std::string suffix_;

public:
	explicit DDLPrinter(const types::Struct& dbc) {
		const auto name = dbc.alias.empty()? dbc.name : dbc.alias;
		out_ << "CREATE TABLE `" << pascal_to_underscore(name) << "` (" << "\n";
	}

	void visit(const types::Struct* structure, const types::Field* parent) override {
		walk_dbc_fields(*this, structure, parent);
	}

	void visit(const types::Enum* enumeration) override {
		// we don't care about enums at the moment
	}

	std::string generate_column(const std::string& name, const std::string& type) {
		std::string column = "  `" + name + "` ";

		if(type == "int8") {
			column +=  "tinyint(1) NOT NULL";
		} else if(type == "uint8") {
			column += "tinyint unsigned NOT NULL";
		} else if(type == "int16") {
			column += "smallint NOT NULL";
		} else if(type == "uint16") {
			column += "smallint unsigned NOT NULL";
		} else if(type == "uint32") {
			column += "int unsigned NOT NULL";
		} else if(type == "int32") {
			column += "int NOT NULL";
		} else if(type == "bool" || type == "bool32") {
			column += "bit(1) NOT NULL";
		} else if(type == "float") {
			column += "float NOT NULL";
		} else if(type == "double") {
			column += "double NOT NULL";
		} else if(type == "string_ref") {
			column += "mediumtext NOT NULL";
		} else if(type == "string_ref_loc") {
			// special case handling :()
			column.clear();
	
			for(const auto& loc : string_ref_loc_regions) {
				column += "  `" + name + "_" + std::string(loc) + "` ";
				column += "mediumtext NOT NULL,\n";
			}

			column += "  `" + name + "_flags` ";
			column += "int NOT NULL";
		}

		return column;
	}

	void visit(const types::Field* field, const types::Base* parent) override {
		const auto name = suffix_.empty()? field->name : field->name + "_" + suffix_;
		const auto qualified_name = parent? parent->name + "_" + name : name;
		const auto& type = field->underlying_type;
		const auto components = extract_components(field->underlying_type);

		// handle primitive types
		if(type_map.contains(components.first)) {
			if(components.second) { // array
				for(auto i = 0u; i < *components.second; ++i) {
					auto column = std::move(generate_column(qualified_name + "_" + std::to_string(i), components.first));
					columns_.emplace_back(std::move(column));
				}
			} else { // scalar
				auto column = std::move(generate_column(qualified_name, type));

				// only scalar primitive types can have keys
				for(const auto& key : field->keys) {
					if(key.type == "primary") {
						column += ", \n  PRIMARY KEY(`" + field->name + "`)";
					} else if (key.type == "foreign") {
						column += " COMMENT 'References " + key.parent + "'";
					}
				}

				columns_.emplace_back(std::move(column));
			}
		} else { // handle user-defined types
			const auto found = locate_type_base(static_cast<types::Struct&>(*field->parent), components.first);

			if(!found) {
				throw std::runtime_error("Unable to locate type base");
			}

			if(found->type == types::Type::STRUCT) {
				if(components.second) {
					for(auto i = 0u; i < *components.second; ++i) {
						suffix_ = std::to_string(i);
						visit(static_cast<const types::Struct*>(found), field);
					}

					suffix_.clear();
				} else {
					visit(static_cast<const types::Struct*>(found), field);
				}
			} else if(found->type == types::Type::ENUM) {
				const auto type = static_cast<const types::Enum*>(found);
				auto i = 0u;

				do {
					const auto& qns = components.second? qualified_name + "_" + std::to_string(i) : qualified_name;
					auto column = std::move(generate_column(qns, type->underlying_type));
					columns_.emplace_back(std::move(column));
					++i;
				} while(components.second && i < *components.second);
			} else {
				throw std::runtime_error("Unhandled type");
			}
		}
	}

	void finalise() {
		bool first_col = true;

		for(const auto& column : columns_) {
			if(!first_col) {
				out_ << "," << "\n";
			}

			first_col = false;
			out_ << column;
		}

		out_ << "\n) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;" << "\n\n";
	}

	const std::stringstream& output() {
		return out_;
	}
};

void write_dbc_ddl(const types::Struct& dbc, std::ofstream& out) {
	DDLPrinter printer(dbc);
	
	if(dbc.fields.empty()) {
		LOG_WARN_GLOB << "Skipping SQL generation for " << dbc.name << " - table cannot have no columns" << LOG_SYNC;
		return;
	}

	walk_dbc_fields(printer, &dbc, dbc.parent);
	printer.finalise();
	out << printer.output().str();
}

void generate_sql_ddl(const types::Definitions& defs, const std::string& out_path) {
	std::ofstream file(out_path + "dbc_schema.sql");

	if(!file) {
		throw std::runtime_error("Unable to open file for SQL DDL generation");
	}
	
	for(const auto& def : defs) {
		if(def->type != types::Type::STRUCT) {
			continue;
		}

		LOG_DEBUG_GLOB << "Generating SQL DDL for " << def->name << LOG_ASYNC;
		write_dbc_ddl(static_cast<const types::Struct&>(*def.get()), file);
		LOG_DEBUG_GLOB << "Completed SQL DDL generation for " << def->name << LOG_ASYNC;
	}

	LOG_DEBUG_GLOB << "Completed SQL DDL generation" << LOG_ASYNC;
}

} // dbc, ember