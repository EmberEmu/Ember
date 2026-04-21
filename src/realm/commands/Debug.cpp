/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Debug.h"
#include "../ServiceContextImpl.h"
#include <allocators/Tracking.h>
#include <logger/Logger.h>
#include <shared/utility/Utility.h>
#include <bprinter/table_printer.h>
#include <string>
#include <sstream>

namespace ember::realm {

namespace {

void print_allocator_debug_table(log::Logger& logger) {
	std::stringstream stream;
	bprinter::TablePrinter table(&stream);
	table.AddColumn("tag", 27);
	table.AddColumn("create", 7);
	table.AddColumn("destroy", 8);
	table.AddColumn("bytes_alloc", 12);
	table.AddColumn("bytes_dealloc", 14);
	table.AddColumn("allocs", 7);
	table.AddColumn("deallocs", 9);
	table.AddColumn("sys_allocs", 11);
	table.AddColumn("sys_deallocs", 13);
	table.PrintHeader();

	auto metrics = allocators::tracking::metrics();

	for(const auto& [k, v] : metrics) {
		table << k.substr(0, 27);
		table << v[mem_rep_create];
		table << v[mem_rep_destroy];
		table << v[mem_rep_bytes_alloc];
		table << v[mem_rep_bytes_dealloc];
		table << v[mem_rep_alloc];
		table << v[mem_rep_dealloc];
		table << v[mem_rep_system_alloc];
		table << v[mem_rep_system_dealloc];
	}

	table.PrintFooter();
	LOG_CONSOLE(logger, "Displaying memory allocation statistics\n{}", stream.str());
}

void handle_allocator_debug(const commands::Arguments& args, log::Logger& logger) {
#ifndef EMBER_DEBUG_ALLOCATOR_TRACKING
	LOG_CONERR(logger, "Cannot display statistics, not built with allocator tracking");
#else
	if(args.contains("file")) {
		const auto file = args["file"].as<std::string>();
		LOG_CONSOLE(logger, "Writing report to {}", file);
		allocators::tracking::generate_report(file);
	}

	print_allocator_debug_table(logger);
#endif
}

auto allocator_debug(utility::CommandExecutor& exec, log::Logger& logger) -> std::shared_ptr<commands::Command> {
	return commands::create("allocator")
		->description("Display allocator statistics")
		->argument<std::string>("file", commands::optional)
		->handler(exec([&](const commands::Arguments& args) {
			handle_allocator_debug(args, logger);
		}
	));
}

} // unnamed

void add_debug_commands(ServiceContext& context, commands::Command& registry, log::Logger& logger) {
	auto impl = context.get();

	auto root = commands::create("debug")
		->description("Misc. debugging commands");

	root->insert(allocator_debug(*impl->cmd_exec, logger));
	impl->commands.emplace_back(registry.scoped_insert(std::move(root)));
}

} // realm, ember