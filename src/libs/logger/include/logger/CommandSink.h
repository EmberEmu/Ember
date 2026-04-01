/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Sink.h>
#include <commands/Suggestions.h>
#include <logger/ConsoleColour.h>
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

	using CommandHandler = std::function<void(const std::string_view)>;
	using Autocomplete = std::function<commands::Suggestions(const std::string&)>;
	using Suggestion = std::function<std::string(const std::string_view)>;

	inline static std::atomic_bool exists_ = false;
	static constexpr auto sv_reserve = 256u;
	static constexpr auto max_buf_size = 4096u;
	static constexpr auto reserve_buf_size = 1024u;
	static constexpr auto history_size = 10u;
	static constexpr auto table_name_min_cols = 20u;
	static constexpr auto table_desc_min_cols = 10u;
	static constexpr auto table_min_cols = 35u;
	static constexpr auto table_max_cols = 100u;
	static constexpr auto table_padding = 3u;

	CommandHandler handler_;
	Autocomplete autocomplete_;
	Suggestion suggestion_;

	std::mutex handler_lock_;
	std::recursive_mutex console_lock_;
	std::string command_;
	std::string suggested_;
	std::string prompt_;
	std::string prefix_;
	std::jthread event_handler_;
	std::atomic_bool stopped_;
	bool colour_;
	unsigned int max_cols_;

	std::deque<std::string> cmd_history_;
	std::size_t history_idx_;

	std::string_view suggestion_substring();
	Colour severity_colour(Severity severity);
	boost::container::small_vector<char, sv_reserve> out_buf_;
	void print_command_table(std::span<const commands::Suggestions::Record> matches);
	std::string truncate_description(int cols, const std::string_view description);

	void clear_line();
	void redraw_prompt();
	void history_scroll(ScrollDirection dir);
	void buffer_scroll(CursorDirection dir);
	void insert_character(char ch);
	void delete_character(bool after);
	void cursor_reposition(CursorPosition position);
	void insert_history(const std::string& command);
	void update_suggestion();
	void read_console_input();
	void dispatch_command();
	void write_buffer(std::span<const char> buffer, bool redraw = true);
	void do_batch_write(const std::span<std::pair<RecordDetail, std::vector<char>>>& records);
	void autocomplete();
	bool invoke_handler(const std::string_view command);

public:
	static constexpr std::string_view sink_name = "CommandSink";

	CommandSink(Severity severity, Filter filter, std::string prompt);
	~CommandSink();

	void stop();
	void write(Severity severity, Filter type, std::span<const char> record, bool flush) override;
	void batch_write(const std::span<std::pair<RecordDetail, std::vector<char>>>& records) override;
	void colourise(bool colourise) { colour_ = colourise; }
	void prefix(std::string prefix) { prefix_ = std::move(prefix); }

	void register_handler(CommandHandler handler);
	void register_autocomplete(Autocomplete handler);
	void register_suggestion(Suggestion handler);

	void clear_console();
	void set_max_table_cols(unsigned int cols);
	bool invoke(const std::string_view command);

	bool unique() override { return true; }
};

} // log, ember