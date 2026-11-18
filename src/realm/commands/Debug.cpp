/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Debug.h"
#include "../ServiceContextImpl.h"
#include <allocators/Tracking.h>
#include <logger/Logger.h>
#include <shared/utility/Utility.h>
#include <bprinter/table_printer.h>
#include <latch>
#include <span>
#include <sstream>
#include <string>
#include <vector>

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

	const auto metrics = allocators::tracking::metrics();

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
	LOG_CONSOLE(logger, "Displaying memory allocation statistics\n{}", stream.view());
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

std::vector<DispatcherStats> handle_client_table_debug(const commands::Arguments& args, thread::ServicePool* pool) {
    std::vector<DispatcherStats> stats(pool->size());

    std::latch latch(pool->size());

    for(std::size_t i = 0; i < pool->size(); ++i) {
        boost::asio::dispatch(pool->get(i), [&, i]() {
            stats[i] = EventDispatcher::statistics();
			latch.count_down();
        });
    }

	latch.wait();
    return stats;
}

void print_client_table_debug(const std::span<const DispatcherStats> stats, log::Logger& logger) {
	std::stringstream stream;
	bprinter::TablePrinter table(&stream);
	table.AddColumn("worker", 8);
	table.AddColumn("table used", 12);
	table.AddColumn("table capacity", 15);
	table.AddColumn("map used", 12);
	table.AddColumn("peak clients", 15);
	table.PrintHeader();

	std::size_t worker = 0;

	for(const auto& s : stats) {
		table << worker;
#ifndef DISABLE_FAST_DISPATCH_TABLE
		table << s.table_occupied;
		table << s.table_capacity;
#else
		table << "disabled";
		table << "disabled";
#endif
		table << s.map_occupied;
		table << s.peak_clients;
		++worker;
	}

	table.PrintFooter();
	LOG_CONSOLE(logger, "Displaying client table statistics\n{}", stream.view());
}

} // unnamed

void add_debug_commands(ServiceContext& context, commands::Command& registry, log::Logger& logger) {
	auto impl = context.get();
	auto& exec = *impl->cmd_exec;

	auto root = commands::create("debug")
		->description("Misc. debugging commands");

	root->insert("table")
		->description("Display client table")
		->handler(exec([&, &context = context](const commands::Arguments& args) {
			const auto stats = handle_client_table_debug(args, context.get()->service_pool);
			print_client_table_debug(stats, logger);
		})
	);

	root->insert("allocator")
		->description("Display allocator statistics")
		->argument<std::string>("file", commands::optional)
		->handler(exec([&](const commands::Arguments& args) {
			handle_allocator_debug(args, logger);
		})
	);

	auto scoped = registry.scoped_insert(root);
	impl->commands.emplace_back(std::move(scoped));
}

} // realm, ember