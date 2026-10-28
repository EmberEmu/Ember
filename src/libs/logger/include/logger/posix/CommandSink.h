/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <commands/Suggestions.h>
#include <logger/CommandSink.h>
#include <logger/ConsoleColour.h>
#include <logger/posix/CommandSinkOptions.h>
#include <logger/posix/Themes.h>
#include <logger/posix/TerminalCtl.h>
#include <boost/container/small_vector.hpp>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <cstddef>

namespace ember::log::posix {

class CommandSink final : public log::CommandSink {
	static constexpr std::string_view sink_name { "PosixCommandSink" };

	enum class ScrollDirection {
		up, down
	};

	enum class CursorDirection {
		back, forward
	};

	enum class CursorPosition {
		start, end
	};

	static constexpr auto sv_reserve = 256u;
	static constexpr auto max_buf_size = 4096u;
	static constexpr auto reserve_buf_size = 512u;
	static constexpr auto history_size = 10u;
	static constexpr auto table_name_min_cols = 20u;
	static constexpr auto table_desc_min_cols = 10u;
	static constexpr auto table_min_cols = 35u;
	static constexpr auto table_max_cols = 100u;
	static constexpr auto table_padding = 3u;
	static constexpr auto timestamp_buf_size = 64u;
	static constexpr auto max_command_length = 256u;

	CommandSinkOptions opts_;

	CommandHandler handler_;
	Autocomplete autocomplete_;
	Suggestion suggestion_;

	std::mutex handler_lock_;
	std::recursive_mutex console_lock_;

	std::string command_;
	std::string suggested_;
	std::string prompt_;
	std::string timestamp_fmt_;
	std::jthread input_reader_;
	TerminalCtl terminal_;
	bool log_line_used_ = false;
	std::string prefix_pad_;
	unsigned int max_cols_ = table_max_cols;
	std::size_t cursor_pos_ = 0;

	std::deque<std::string> cmd_history_;
	std::size_t history_idx_ = 0;

	boost::container::small_vector<char, sv_reserve> out_buf_;

	std::string_view suggestion_substring();

	void print_command_table(std::span<const commands::Suggestions::Record> matches);
	std::size_t generate_timestamp(std::span<char> buffer);

	void clear_line();
	void redraw_prompt();

	void history_scroll(ScrollDirection dir);
	void buffer_scroll(CursorDirection dir);
	void insert_character(char ch);
	void delete_character(bool after);
	void cursor_reposition(CursorPosition position);
	void insert_history(const std::string& command);
	void update_suggestion();

	void read_console_input(std::stop_token stop);
	void dispatch_command();

	void write_buffer(std::span<const char> buffer, bool redraw = true);
	void do_batch_write(const std::span<std::tuple<RecordDetail, std::string_view, std::string>>& records);

	void autocomplete();
	bool invoke_handler(std::string_view command);

public:
	explicit CommandSink(CommandSinkOptions options);
	~CommandSink();

	CommandSink(const CommandSink&) = delete;
	CommandSink& operator=(const CommandSink&) = delete;

	void stop();

	void write(Severity severity, Filter type, std::string_view prefix, std::string_view record, bool flush) override;
	void batch_write(const std::span<std::tuple<RecordDetail, std::string_view, std::string>>& records) override;

	// void prefix_width(unsigned int width) { prefix_pad_ = std::string(width, ' '); } - todo
	void set_theme(Theme theme);

	void register_handler(CommandHandler handler) override;
	void register_autocomplete(Autocomplete handler) override;
	void register_suggestion(Suggestion handler) override;

	void clear_console() override;
	void set_max_table_cols(unsigned int cols);
	bool invoke(std::string_view command) override;

	bool unique() override { return true; }

	static constexpr std::string_view name() { return sink_name; }
};

} // posix, log, ember