/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Sink.h>
#include <shared/commands/Suggestions.h>
#include <shared/utility/ConsoleColour.h>
#include <boost/container/small_vector.hpp>
#include <atomic>
#include <deque>
#include <functional>
#include <mutex>
#include <span>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace ember::log {

class CommandSink final : public Sink {
	enum class ScrollDirection {
		up, down
	};

	enum class CursorDirection {
		back, forward
	};

	enum class CursorPosition {
		start, end
	};

	using CommandHandler = std::function<void(std::string_view)>;
	using Autocomplete = std::function<commands::Suggestions(const std::string&)>;

	static constexpr auto sv_reserve = 256u;
	static constexpr auto max_buf_size = 4096u;
	static constexpr auto reserve_buf_size = 1024u;
	static constexpr auto history_size = 5u;
	static constexpr auto table_name_cols = 20u;
	static constexpr auto table_desc_cols = 50u;

	static inline std::mutex colour_lock;

	CommandHandler handler_;
	Autocomplete autocomplete_;

	std::mutex handler_lock_;
	std::recursive_mutex console_lock_;
	std::string command_;
	std::string prompt_;
	std::string prefix_;
	std::jthread event_handler_;
	std::atomic_bool stopped_;
	bool colour_;

	std::deque<std::string> cmd_history_;
	std::size_t history_idx_;

	util::Colour severity_colour(Severity severity);
	boost::container::small_vector<char, sv_reserve> out_buf_;
	void print_command_table(std::span<const commands::Suggestions::Record> matches);
	std::string truncate_description(std::string_view description);

	void clear_line();
	void redraw_prompt();
	void history_scroll(ScrollDirection dir);
	void buffer_scroll(CursorDirection dir);
	void insert_character(char ch);
	void delete_character(bool after);
	void cursor_reposition(CursorPosition position);

	void read_console_input();
	void dispatch_command();
	void write_buffer(std::span<const char> buffer, bool redraw = true);
	void do_batch_write(const std::span<std::pair<RecordDetail, std::vector<char>>>& records);
	void autocomplete();

public:
	CommandSink(Severity severity, Filter filter, std::string prompt);
	~CommandSink();

	void stop();
	void write(Severity severity, Filter type, std::span<const char> record, bool flush) override;
	void batch_write(const std::span<std::pair<RecordDetail, std::vector<char>>>& records) override;
	void colourise(bool colourise) { colour_ = colourise; }
	void prefix(std::string prefix) { prefix_ = std::move(prefix); }

	void register_handler(CommandHandler handler);
	void register_autocomplete(Autocomplete handler);
};

} // log, ember