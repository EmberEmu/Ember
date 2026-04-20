/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <array>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <cstddef>

#ifdef EMBER_DEBUG_ALLOCATOR_TRACKING
#define ALLOC_TRACK(tag, type, ...) \
ember::memory::report::record(tag, type __VA_OPT__(,) __VA_ARGS__);
#else
#define ALLOC_TRACK(tag, type, ...)
#endif

enum Metric {
	mem_rep_create,
	mem_rep_destroy,
	mem_rep_bytes_alloc,
	mem_rep_bytes_dealloc,
	mem_rep_alloc,
	mem_rep_dealloc,
	mem_rep_system_alloc,
	mem_rep_system_dealloc,

	_metric_max
};

namespace ember::memory::report {

class Tracking {
public:
	using Metrics = std::array<std::size_t, _metric_max>;
	using MetricsMap = std::unordered_map<std::string, Metrics>;

private:
	MetricsMap metrics;
	std::mutex lock;

	Tracking() = default;

	Metrics& get_tag_metrics(std::string_view tag) {
		return metrics[std::string(tag)];
	}

public:
	static Tracking& get_instance() {
		static Tracking instance;
		return instance;
	}

	MetricsMap get_metrics() {
		std::unique_lock guard(lock);
		return metrics;
	}
	
	void record(std::string_view tag, Metric type, std::size_t value) {
		std::unique_lock guard(lock);
		auto& metrics = get_tag_metrics(tag);
		metrics[type] += value;
	}

	void generate_report(const std::string& file) const {
		std::ofstream stream(file);

		if(!stream) {
			return;
		}

		for(auto& [k, v] : metrics) {
			stream << "===================================\n";
			stream << k << '\n';
			stream << "===================================\n";
			stream << "create: "               << v[mem_rep_create]         << '\n';
			stream << "destroy: "              << v[mem_rep_destroy]        << '\n';
			stream << "bytes_allocated: "      << v[mem_rep_bytes_alloc]    << '\n';
			stream << "bytes_deallocated: "    << v[mem_rep_bytes_dealloc]  << '\n';
			stream << "allocations: "          << v[mem_rep_alloc]          << '\n';
			stream << "deallocations: "        << v[mem_rep_dealloc]        << '\n';
			stream << "system_allocations: "   << v[mem_rep_system_alloc]   << '\n';
			stream << "system_deallocations: " << v[mem_rep_system_dealloc] << "\n\n";
		}
	}

	~Tracking() {
		generate_report("report.txt");
	}
};

inline void record(std::string_view tag, Metric type, std::size_t value = 1) {
	Tracking::get_instance().record(tag, type, value);
}

inline void generate_report(const std::string& file) {
	Tracking::get_instance().generate_report(file);
}

inline Tracking::MetricsMap metrics() {
	return Tracking::get_instance().get_metrics();
}

}; // memory, report, ember