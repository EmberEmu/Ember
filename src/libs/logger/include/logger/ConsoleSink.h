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
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace ember::log {

class ConsoleSink final : public Sink {
	static constexpr std::string_view sink_type { "ConsoleSink" };
	static constexpr std::string_view default_timestamp_fmt { "%d/%m/%Y %H:%M:%S " };

	static constexpr auto sv_reserve = 256u;
	static constexpr auto max_buf_size = 4096u;
	static constexpr auto timestamp_buf_size = 64u;
	static inline std::mutex colour_lock;

	bool colour_;
	bool timestamp_;

	std::string timestamp_fmt_;

	boost::container::small_vector<char, sv_reserve> out_buf_;

	std::size_t generate_timestamp(std::span<char> buffer);
	void do_batch_write(const std::span<std::pair<RecordDetail, std::string>>& records);

public:
	ConsoleSink(std::string name, Severity severity, Filter filter)
		: Sink(std::move(name), std::string(type()), severity, filter)
		, colour_(false)
		, timestamp_(false) {}

	void write(Severity severity, Filter type, std::string_view record, bool flush) override;
	void batch_write(const std::span<std::pair<RecordDetail, std::string>>& records) override;
	void colourise(bool colourise) { colour_ = colourise; }
	void log_timestamp(bool enable) { timestamp_ = enable; }
	void timestamp_format(std::string format) { timestamp_fmt_ = format; }

	static constexpr std::string_view type() { return sink_type; }
};

} // log, ember