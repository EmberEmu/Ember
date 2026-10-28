/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <logger/win32/CommandSink.h>
#include <logger/Exception.h>
#include <logger/Utility.h>
#include <logger/detail/StreamBuffer.h>
#include <shared/utility/Truncate.h>
#include <bprinter/table_printer.h>
#include <gsl/narrow>
#include <boost/algorithm/string.hpp>
#include <boost/container/small_vector.hpp>
#include <shared/utility/FormatPacket.h>
#include <Windows.h>
#include <algorithm>
#include <array>
#include <iostream>
#include <iterator>
#include <string_view>
#include <cassert>
#include <cstdio>
#include <cstring>

namespace ember::log::win32 {

using namespace detail;

static constexpr auto text_white = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
static constexpr auto text_grey = FOREGROUND_INTENSITY;

CommandSink::CommandSink(CommandSinkOptions options)
	: log::CommandSink(options.name, options.severity, options.filter)
	, opts_(std::move(options))
	, prefix_pad_(std::string(' ', opts_.prefix_width))
	, history_idx_(0)
	, max_cols_(table_max_cols) {
	semaphore_ = CreateSemaphore(NULL, 0, 1, NULL);

	if(!semaphore_) {
		throw std::runtime_error("Unable to initialise command sink, semaphore create failed");
	}

	input_reader_ = std::jthread([&](std::stop_token stop) {
		redraw_prompt();
		read_console_input(stop);
	});
}

CommandSink::~CommandSink() {
	stop();

	if(input_reader_.joinable()) {
		input_reader_.join();
	}

	CloseHandle(semaphore_);
}

void CommandSink::stop() {
	input_reader_.request_stop();
	ReleaseSemaphore(semaphore_, 1, NULL);
}

void CommandSink::batch_write(const std::span<std::tuple<RecordDetail, std::string_view, std::string>>& records) {
	if(!opts_.colours) [[unlikely]] {
		do_batch_write(records);
	} else { // we can't do batch output if we need to colour each individual log record
		for(auto& [detail, prefix, data] : records) {
			write(detail.severity, detail.type, prefix, data, false);
		}
	}
}

void CommandSink::do_batch_write(const std::span<std::tuple<RecordDetail, std::string_view, std::string>>& records) {
	std::size_t size = 0;
	Severity sink_sev = this->severity();
	Filter sink_filter = this->filter();
	bool matches = false;

	for(auto&& [detail, prefix, data] : records) {
		if(sink_sev <= detail.severity && !(sink_filter & detail.type)) {
			size += data.size();
			matches = true;
		}
	}

	if(!matches) {
		return;
	}

	std::lock_guard guard(console_lock_);
	out_buf_.reserve(size + (10 * records.size()));

	[[indeterminate]] std::array<char, timestamp_buf_size> ts_buffer;
	std::size_t ts_size = 0;

	if(opts_.timestamp) {
		ts_size = generate_timestamp(ts_buffer);
	}

	std::span<char> timestamp(ts_buffer.data(), ts_size);

	for(auto&& [detail, prefix, data] : records) {
		if(sink_sev > detail.severity || (sink_filter & detail.type)) {
			continue;
		}

		std::string_view severity = severity_string(detail.severity);
		const auto cur_sz = out_buf_.size();

		// todo, size_t literal on bump
		const auto padding = !prefix.empty()?
			std::max<std::size_t>(1u, prefix_pad_.size() > prefix.size()?
				prefix_pad_.size() - prefix.size() : 0u) : 0u;

		const auto req_buf_size
			= cur_sz
			+ timestamp.size()
			+ prefix.size()
			+ padding
			+ severity.size()
			+ data.size();

		out_buf_.resize(req_buf_size, boost::container::default_init);
		auto write_ptr = out_buf_.data() + cur_sz;
		std::size_t offset = 0;

		std::memcpy(write_ptr + offset, timestamp.data(), timestamp.size());
		offset += timestamp.size();

		std::memcpy(write_ptr + offset, prefix.data(), prefix.size());
		offset += prefix.size();

		std::memcpy(write_ptr + offset, prefix_pad_.data(), padding);
		offset += padding;

		std::memcpy(write_ptr + offset, severity.data(), severity.size());
		offset += severity.size();

		std::memcpy(write_ptr + offset, data.data(), data.size());
	}

	write_buffer(out_buf_, false);
	redraw_prompt(false);
	out_buf_.clear();

	if(out_buf_.capacity() > max_buf_size) [[unlikely]] {
		out_buf_.shrink_to_fit();
	}
}

void CommandSink::write(Severity severity, Filter type, std::string_view prefix, std::string_view record, bool flush) {
	if(this->severity() > severity || (this->filter() & type)) {
		return;
	}

	std::lock_guard guard(console_lock_);

	[[indeterminate]] std::array<char, timestamp_buf_size> ts_buffer;
	std::size_t ts_size = 0;

	if(opts_.timestamp) {
		ts_size = generate_timestamp(ts_buffer);
	}

	std::span<char> timestamp(ts_buffer.data(), ts_size);
	std::string_view sevsv = severity_string(severity);

	// todo, size_t literal on bump
	const auto padding = !prefix.empty()?
		std::max<std::size_t>(1u, prefix_pad_.size() > prefix.size()?
			prefix_pad_.size() - prefix.size() : 0u) : 0u;

	const auto buff_req_size
		= timestamp.size()
		+ prefix.size()
		+ padding
		+ record.size()
		+ sevsv.size();

	out_buf_.resize(buff_req_size, boost::container::default_init);

	std::size_t offset = 0;

	const auto append = [&](auto&& fragment) {
		if(fragment.size() > out_buf_.size() - offset) {
			throw std::runtime_error("output buffer full, bad calculation");
		}

		std::copy(fragment.begin(), fragment.end(), out_buf_.begin() + offset);
		offset += fragment.size();
	};

	append(timestamp);
	append(prefix);
	append(std::string_view(prefix_pad_.data(), padding));
	append(sevsv);
	append(record);

	if(offset != buff_req_size) {
		throw std::runtime_error("output buffer size != offset, bad calculation");
	}

	if(opts_.colours) [[likely]] {
		std::lock_guard guard(console_lock_);
		ConsoleColour concol(detail::severity_colour(severity));
		write_buffer(out_buf_, false);
	} else {
		write_buffer(out_buf_, false);
	}

	redraw_prompt(false);

	if(flush) {
		if(std::fflush(stdout) != 0) {
			throw exception("Unable to flush log record to console");
		}
	}

	out_buf_.clear();

	if(out_buf_.capacity() > max_buf_size) [[unlikely]] {
		out_buf_.shrink_to_fit();
	}
}

void CommandSink::set_max_table_cols(const unsigned int cols) {
	if(cols < table_min_cols) {
		max_cols_ = table_min_cols;
	} else {
		max_cols_ = cols;
	}
}

void CommandSink::clear_line() {
	auto handle = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO info{};
	GetConsoleScreenBufferInfo(handle, &info);

	const SHORT window_width = info.srWindow.Right - info.srWindow.Left + 1;
	const SHORT bottom_row = info.srWindow.Bottom;
	const COORD startPos {
		.X = 0, .Y = bottom_row 
	};

	DWORD written = 0;

	FillConsoleOutputCharacter(handle, ' ',window_width, startPos, &written);
	FillConsoleOutputAttribute(handle, info.wAttributes, window_width, startPos, &written);
}

std::string_view CommandSink::suggestion_substring() {
	if(auto pos = suggested_.find(command_); pos != std::string::npos) {
		return std::string_view(suggested_).substr(pos + command_.length());
	} else {
		return {};
	}
}

std::size_t CommandSink::generate_timestamp(std::span<char> buffer) {
	const auto curr_time = detail::current_time();
	const auto size = detail::put_time(curr_time, timestamp_fmt_, buffer);

	if(!size) [[unlikely]] {
		throw std::runtime_error("timestamp formatting failed");
	}

	if(size >= buffer.size()) [[unlikely]] {
		throw std::runtime_error("timestamp buffer out of space");
	}

	buffer[size] = ' '; // add a trailing space
	return size + 1;    // we don't need the null terminator
}

void CommandSink::redraw_prompt(bool update_cursor) {
    std::lock_guard guard(console_lock_);

    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO info{};
    GetConsoleScreenBufferInfo(handle, &info);

    const SHORT width = info.dwSize.X;

	// process command
	const auto suggested = suggestion_substring();
	const auto clamp = min(width - opts_.prompt.size() - 1, command_.size());
	const auto user_input = std::format("{}{}", command_, suggested);
    const std::string full_input = std::format("{}{}", opts_.prompt, user_input);

	[[indeterminate]] boost::container::small_vector<CHAR_INFO, reserve_buf_size> buffer(
		width, boost::container::default_init
	);

    for(SHORT i = 0; i < width; ++i) {
        buffer[i].Char.AsciiChar = ' ';
        buffer[i].Attributes = info.wAttributes;
    }

	for(std::size_t i = 0; i < full_input.size() && i < width; ++i) {
		buffer[i].Char.AsciiChar = full_input[i];
	
		if(opts_.colours) {
			if(i < opts_.prompt.size()) {
				buffer[i].Attributes = text_grey;
			} else if(i < opts_.prompt.size() + command_.size()) {
				buffer[i].Attributes = text_white;
			} else {
				buffer[i].Attributes = text_grey;
			}
		}
	}

    const SHORT bottom = info.dwSize.Y - 1;

    SMALL_RECT region {
		.Left = 0,
		.Top = bottom,
		.Right = gsl::narrow_cast<SHORT>(width - 1),
		.Bottom = bottom
    };

    const COORD buf_size { 
		.X = width, 
		.Y = 1
	};

    const COORD buf_coord {};

	WriteConsoleOutput(handle, buffer.data(), buf_size, buf_coord, &region);

	if(update_cursor) {
		COORD cursor{
			.X = gsl::narrow_cast<SHORT>(opts_.prompt.size() + clamp),
			.Y = bottom
		};

		SetConsoleCursorPosition(handle, cursor);
	}
}

void CommandSink::insert_history(const std::string& command) {
	// don't insert duplicates of the previous command
	if(!cmd_history_.empty() && cmd_history_.back() == command) {
		return;
	}

	cmd_history_.emplace_back(command);

	if(cmd_history_.size() > history_size) {
		cmd_history_.pop_front();
	}
}

void CommandSink::dispatch_command() {
	boost::trim(command_);

	suggested_.clear();
	
	// if the string was just whitespace
	if(command_.empty()) {
		redraw_prompt();
		return;
	}

	if(!invoke_handler(command_)) {
		std::string_view msg = "Unable to execute command, no command handler has been registered";

		if(opts_.colours) {
			ConsoleColour concol(Colour::white_on_red_bg);
			write_buffer(msg, true);
		} else {
			write_buffer(msg, true);
		}
	}

	insert_history(command_);

	history_idx_ = cmd_history_.size();
	command_.clear();
	redraw_prompt();
}

void CommandSink::history_scroll(const ScrollDirection dir) {
	if(cmd_history_.empty()) {
		return;
	}

	if(dir == ScrollDirection::up) {
		if(history_idx_) {
			--history_idx_;
		}
	} else {
		if(history_idx_ == cmd_history_.size()) {
			--history_idx_;
		} else if(history_idx_ + 1 < cmd_history_.size()) {
			++history_idx_;
		}
	}

	command_ = cmd_history_[history_idx_];
	update_suggestion();
	redraw_prompt();
}

void CommandSink::delete_character(const bool current) {
	auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info{};
	GetConsoleScreenBufferInfo(handle, &info);

	auto rel_pos = info.dwCursorPosition.X - opts_.prompt.size();

	if(current) {
		command_.erase(rel_pos, 1);
	} else if(rel_pos) {
		command_.erase(--rel_pos, 1);
	}

	update_suggestion();
	redraw_prompt();
	info.dwCursorPosition.X = gsl::narrow_cast<SHORT>(opts_.prompt.size() + rel_pos);
	SetConsoleCursorPosition(handle, info.dwCursorPosition);
}

void CommandSink::buffer_scroll(CursorDirection dir) {
	auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info{};
	GetConsoleScreenBufferInfo(handle, &info);

	if(info.dwCursorPosition.X < opts_.prompt.size()) {
		return;
	}

	auto rel_pos = info.dwCursorPosition.X - opts_.prompt.size();

	if(dir == CursorDirection::back && rel_pos) {
		--rel_pos;
	} else if(dir == CursorDirection::forward) {
		++rel_pos;
	}

	// clamp cursor range
	if(rel_pos > command_.size()) {
		rel_pos = command_.size();
	}

	// back to absolute position
	info.dwCursorPosition.X = gsl::narrow_cast<SHORT>(opts_.prompt.size() + rel_pos);
	SetConsoleCursorPosition(handle, info.dwCursorPosition);
}

void CommandSink::insert_character(char ch) {
	auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info{};
	GetConsoleScreenBufferInfo(handle, &info);

	auto rel_pos = info.dwCursorPosition.X - opts_.prompt.size();

	if(rel_pos != command_.size()) {
		command_.insert(rel_pos, &ch, 1);
	} else {
		command_.push_back(ch);
	}

	++rel_pos;

	auto suggest_view = suggestion_substring();

	// prevent calling the handler for every key stroke if we
	// know the result wouldn't change
	if((suggest_view.empty() && !suggested_.empty())
	   || command_.size() == 1
	   || rel_pos != command_.size()) {
		update_suggestion();
	}

	redraw_prompt();

	info.dwCursorPosition.X = gsl::narrow_cast<SHORT>(opts_.prompt.size() + rel_pos);
	SetConsoleCursorPosition(handle, info.dwCursorPosition);
}

void CommandSink::read_console_input(const std::stop_token stop) {
	auto handle = GetStdHandle(STD_INPUT_HANDLE);

	std::array handles {
		handle, semaphore_
	};

	while(!stop.stop_requested()) {
		const auto result = WaitForMultipleObjects(handles.size(), handles.data(), false, INFINITE);
		const auto wait_obj_idx = result - WAIT_OBJECT_0;

		if(wait_obj_idx == 1) { // stop should be signalled
			continue;
		}

		std::array<INPUT_RECORD, 32> events{};
		DWORD event_count = 0;

		ReadConsoleInput(handle, events.data(), events.size(), &event_count);
		
		// not splitting this out because I don't want Windows.h leaking (without splitting impl)
		for(const auto& e : std::span(events.data(), event_count)) {
			if(e.EventType == KEY_EVENT && e.Event.KeyEvent.bKeyDown) {
				if(isprint(e.Event.KeyEvent.uChar.AsciiChar)) {
					insert_character(e.Event.KeyEvent.uChar.AsciiChar);
				} else if(e.Event.KeyEvent.wVirtualKeyCode == VK_TAB) {
					autocomplete();
				} else if(e.Event.KeyEvent.wVirtualKeyCode == VK_BACK) {
					delete_character(false);
				} else if(e.Event.KeyEvent.wVirtualKeyCode == VK_RETURN) {
					dispatch_command();
				} else if(e.Event.KeyEvent.wVirtualKeyCode == VK_DELETE) {
					delete_character(true);
				} else if(e.Event.KeyEvent.wVirtualKeyCode == VK_UP) {
					history_scroll(ScrollDirection::up);
				} else if(e.Event.KeyEvent.wVirtualKeyCode == VK_DOWN) {
					history_scroll(ScrollDirection::down);
				} else if(e.Event.KeyEvent.wVirtualKeyCode == VK_LEFT) {
					buffer_scroll(CursorDirection::back);
				} else if(e.Event.KeyEvent.wVirtualKeyCode == VK_RIGHT) {
					buffer_scroll(CursorDirection::forward);
				} else if(e.Event.KeyEvent.wVirtualKeyCode == VK_HOME) {
					cursor_reposition(CursorPosition::start);
				} else if(e.Event.KeyEvent.wVirtualKeyCode == VK_END) {
					cursor_reposition(CursorPosition::end);
				}
			}
		}
	}
}

void CommandSink::autocomplete() {
	if(!autocomplete_) {
		return;
	}

	const auto result = autocomplete_(command_);

	if(result.records.empty()) {
		return;
	}
	
	std::lock_guard guard(console_lock_);
	command_ = result.substring;
	print_command_table(result.records);
	redraw_prompt();
}

void CommandSink::print_command_table(std::span<const commands::Suggestions::Record> matches) {
	boost::container::small_vector<char, max_buf_size> buffer;
	detail::StreamBuffer stream_buffer(buffer);
	std::ostream stream(&stream_buffer);

	auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info{};
	GetConsoleScreenBufferInfo(handle, &info);

	const int columns = max(min(info.dwSize.X, max_cols_) - table_padding, table_min_cols - table_padding);
	const int table_name_cols = max(columns / 3, table_name_min_cols);
	const int table_desc_cols = max(columns - table_name_cols, table_desc_min_cols);

	bprinter::TablePrinter printer(&stream);
	printer.AddColumn("Command", table_name_cols);
	printer.AddColumn("Description", table_desc_cols);
	printer.PrintHeader();

	for(const auto& match : matches) {
		utility::write_pretty_truncate(printer, match.name, table_name_cols);
		utility::write_pretty_truncate(printer, match.desc, table_desc_cols);

	}

	printer.PrintFooter();
	write_buffer(buffer, true);
}

void CommandSink::cursor_reposition(const CursorPosition position) {
	auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info{};
	GetConsoleScreenBufferInfo(handle, &info);

	if(position == CursorPosition::start) {
		info.dwCursorPosition.X = gsl::narrow_cast<SHORT>(opts_.prompt.size());
	} else if(position == CursorPosition::end) {
		info.dwCursorPosition.X = gsl::narrow_cast<SHORT>(opts_.prompt.size() + command_.size());
	}

	SetConsoleCursorPosition(handle, info.dwCursorPosition);
}

static std::wstring utf8_to_utf16(const std::string_view str) {
	if(str.empty()) {
		return {};
	}

	auto size_needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), gsl::narrow_cast<int>(str.size()), nullptr, 0);
	std::wstring wstr(size_needed, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), wstr.data(), size_needed);
	return wstr;
}

void CommandSink::clear_console() {
	std::lock_guard guard(console_lock_);

	COORD topLeft{};
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO screen;
	DWORD written{};
	
	GetConsoleScreenBufferInfo(console, &screen);

	FillConsoleOutputCharacterA(
		console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
	);

	FillConsoleOutputAttribute(
		console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
		screen.dwSize.X * screen.dwSize.Y, topLeft, &written
	);

	SetConsoleCursorPosition(console, topLeft);
	redraw_prompt();
}

void CommandSink::write_buffer(std::span<const char> buffer, bool redraw) {
	std::lock_guard guard(console_lock_);

	if(buffer.empty()) {
		return;
	}

	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info{};

	if(!GetConsoleScreenBufferInfo(handle, &info)) {
		return;
	}

	const auto width = info.dwSize.X;
	const auto prompt_row = info.dwCursorPosition.Y;

	const auto wlen = MultiByteToWideChar(
		CP_UTF8, 0,
		reinterpret_cast<const char*>(buffer.data()),
		gsl::narrow_cast<int>(buffer.size()),
		nullptr, 0
	);

	if(wlen <= 0) {
		return;
	}

	[[indeterminate]] boost::container::small_vector<wchar_t, reserve_buf_size> wbuf(
		wlen, boost::container::default_init
	);

	MultiByteToWideChar(
		CP_UTF8, 0,
		reinterpret_cast<const char*>(buffer.data()),
		static_cast<int>(buffer.size()),
		wbuf.data(),
		wlen
	);

	std::size_t row_start = 0;

	while(row_start < wbuf.size()) {
		std::size_t row_end = row_start;
		std::size_t row_len = 0;

		while(row_end < wbuf.size() && row_len < width) {
			if (wbuf[row_end] == L'\n') break;
			++row_end;
			++row_len;
		}

		if(prompt_row > 0) {
			const SMALL_RECT scroll_rect {
				.Right = width - 1,
				.Bottom = prompt_row - 1
			};

			const COORD dest_origin {
				 .Y = -1
			};

			CHAR_INFO fill{};
			fill.Char.UnicodeChar = L' ';
			fill.Attributes = info.wAttributes;
			ScrollConsoleScreenBuffer(handle, &scroll_rect, nullptr, dest_origin, &fill);
		}

		[[indeterminate]] boost::container::small_vector<CHAR_INFO, reserve_buf_size> row(
			width, boost::container::default_init
		);

		for(SHORT i = 0; i < width; ++i) {
			row[i].Char.UnicodeChar = i < row_len? wbuf[row_start + i] : L' ';
			row[i].Attributes = info.wAttributes;
		}

		SMALL_RECT rect {
			.Left = 0, 
			.Top = static_cast<SHORT>(prompt_row - 1),
			.Right = width - 1,
			.Bottom = static_cast<SHORT>(prompt_row - 1)
		};

		const COORD buf_size {
			.X = width,
			.Y = 1
		};

		const COORD buf_coord {};
		WriteConsoleOutputW(handle, row.data(), buf_size, buf_coord, &rect);

		row_start = row_end;

		if(row_start < wbuf.size() && wbuf[row_start] == L'\n') {
			++row_start;
		}
	}

	if(redraw) {
		redraw_prompt();
	}
}

void CommandSink::update_suggestion() {
	std::lock_guard lock(handler_lock_);

	if(suggestion_) {
		suggested_ = suggestion_(command_);
	}
}
void CommandSink::register_handler(CommandHandler handler) {
	std::lock_guard lock(handler_lock_);
	handler_ = std::move(handler);
}

void CommandSink::register_autocomplete(Autocomplete handler) {
	std::lock_guard lock(handler_lock_);
	autocomplete_ = std::move(handler);
}

void CommandSink::register_suggestion(Suggestion handler) {
	std::lock_guard lock(handler_lock_);
	suggestion_ = std::move(handler);
}

bool CommandSink::invoke_handler(const std::string_view command) {
	std::lock_guard lock(handler_lock_);

	if(handler_) {
		handler_(command);
		return true;
	}
	
	return false;
}

bool CommandSink::invoke(const std::string_view command) {
	return invoke_handler(command);
}

} // win32, log, ember