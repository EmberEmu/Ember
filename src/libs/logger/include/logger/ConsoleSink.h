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
#include <logger/ConsoleSinkOptions.h>
#include <boost/container/small_vector.hpp>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace ember::log {

class ConsoleSink final : public Sink {
	static constexpr std::string_view sink_type { "ConsoleSink" };

	static constexpr auto sv_reserve = 256u;
	static constexpr auto max_buf_size = 4096u;
	static constexpr auto timestamp_buf_size = 64u;
	static inline std::mutex colour_lock;

	const ConsoleSinkOptions opts_;
	std::string prefix_pad_;

	boost::container::small_vector<char, sv_reserve> out_buf_;

	std::size_t generate_timestamp(std::span<char> buffer);
	void do_batch_write(const std::span<std::tuple<RecordDetail, std::string_view, std::string>>& records);

public:
	ConsoleSink(ConsoleSinkOptions options);

	void write(Severity severity, Filter type, std::string_view prefix, std::string_view record, bool flush) override;
	void batch_write(const std::span<std::tuple<RecordDetail, std::string_view, std::string>>& records) override;

	static constexpr std::string_view type() { return sink_type; }
};

} // log, ember