/*
 * Copyright (c) 2015 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Sink.h>
#include <logger/ConsoleColour.h>
#include <boost/container/small_vector.hpp>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>

namespace ember::log {

class ConsoleSink final : public Sink {
	static constexpr auto SV_RESERVE = 256u;
	static constexpr auto MAX_BUF_SIZE = 4096u;
	static inline std::mutex colour_lock;

	bool colour_;
	std::string prefix_;
	boost::container::small_vector<char, SV_RESERVE> out_buf_;

	Colour severity_colour(Severity severity);
	void do_batch_write(const std::span<std::pair<RecordDetail, std::vector<char>>>& records);

public:
	static constexpr std::string_view sink_name = "ConsoleSink";

	ConsoleSink(Severity severity, Filter filter)
		: Sink(severity, filter, "ConsoleSink")
		, colour_(false) {}

	void write(Severity severity, Filter type, std::span<const char> record, bool flush) override;
	void batch_write(const std::span<std::pair<RecordDetail, std::vector<char>>>& records) override;
	void colourise(bool colourise) { colour_ = colourise; }
	void prefix(std::string prefix) { prefix_ = std::move(prefix); }
};

} // log, ember