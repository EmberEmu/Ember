/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <logger/CommandSink.h>
#include <logger/Exception.h>
#include <logger/Utility.h>
#include <logger/detail/StreamBuffer.h>
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
#include <cstdio>
#include <cstring>

namespace ember::log {

using namespace detail;

static constexpr auto prompt_colour = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // white

CommandSink::CommandSink(Severity severity, Filter filter, std::string prompt)
	: Sink(severity, filter, name),
	  prompt_(std::move(prompt)),
	  colour_(false),
	  stopped_(false),
	  history_idx_(0) {
	if(exists_) {
		throw std::runtime_error("A process cannot have multiple CommandSinks!");
	} else {
		exists_ = true;
	}

	event_handler_ = std::jthread([&]() {
		redraw_prompt();
		read_console_input();
	});
}

CommandSink::~CommandSink() {
	stop();
	exists_ = false;
}

void CommandSink::stop() {
	stopped_ = true;
}

void CommandSink::batch_write(const std::span<std::pair<RecordDetail, std::vector<char>>>& records) {
	if(!colour_) [[unlikely]] {
		do_batch_write(records);
	} else { // we can't do batch output if we need to colour each individual log record
		for(auto& [detail, data] : records) {
			write(detail.severity, detail.type, data, false);
		}
	}
}

void CommandSink::do_batch_write(const std::span<std::pair<RecordDetail, std::vector<char>>>& records) {
	std::size_t size = 0;
	Severity sink_sev = this->severity();
	Filter sink_filter = this->filter();
	bool matches = false;

	for(auto&& [detail, data] : records) {
		if(sink_sev <= detail.severity && !(sink_filter &detail.type)) {
			size += data.size();
			matches = true;
		}
	}

	if(!matches) {
		return;
	}

	out_buf_.reserve(size + (10 * records.size()));

	const bool prefix = !prefix_.empty();

	for(auto&& [detail, data] : records) {
		if(sink_sev <= detail.severity && !(sink_filter & detail.type)) {
			std::string_view severity = detail::severity_string(detail.severity);
			const auto cur_sz = out_buf_.size();
			const auto new_sz = cur_sz + severity.size() + data.size() + prefix_.size();
			out_buf_.resize(new_sz, boost::container::default_init);
			auto write_ptr = out_buf_.data() + cur_sz;
			std::size_t offset = 0;

			if(prefix) {
				std::memcpy(write_ptr + offset, prefix_.data(), prefix_.size());
				offset += prefix_.size();
			}

			std::memcpy(write_ptr + offset, severity.data(), severity.size());
			offset += severity.size();
			std::memcpy(write_ptr + offset, data.data(), data.size());
		}
	}

	write_buffer(out_buf_);
	out_buf_.clear();

	if(out_buf_.capacity() > max_buf_size) [[unlikely]] {
		out_buf_.shrink_to_fit();
	}
}

void CommandSink::write(Severity severity, Filter type, std::span<const char> record, bool flush) {
	if(this->severity() > severity || (this->filter() & type)) {
		return;
	}

	std::string_view sevsv = detail::severity_string(severity);
	out_buf_.resize(prefix_.size() + record.size() + sevsv.size(), boost::container::default_init);

	std::size_t offset = 0;

	const auto append = [&](auto& fragment) {
		std::copy(fragment.begin(), fragment.end(), out_buf_.begin() + offset);
		offset += fragment.size();
	};

	append(prefix_);
	append(sevsv);
	append(record);

	if(colour_) [[likely]] {
		utility::ConsoleColour concol(severity_colour(severity));
		write_buffer(out_buf_, false);
	} else {
		write_buffer(out_buf_, false);
	}

	redraw_prompt();

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

utility::Colour CommandSink::severity_colour(Severity severity) {
	switch(severity) {
		case Severity::console:
			return utility::Colour::white_on_cyan_bg;
		case Severity::console_error:
			return utility::Colour::white_on_red_bg;
		case Severity::fatal:
			[[fallthrough]];
		case Severity::ERROR_:
			[[fallthrough]];
		case Severity::warn:
			return utility::Colour::light_red;
		case Severity::info:
			return utility::Colour::white;
		case Severity::debug:
			return utility::Colour::light_cyan;
		case Severity::trace:
			return utility::Colour::dark_grey;
		case Severity::disabled:
			[[fallthrough]];
		default:
			return utility::Colour::default_colour;
	}
}

void CommandSink::clear_line() {
	auto handle = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO info{};
	GetConsoleScreenBufferInfo(handle, &info);

	const SHORT window_width = info.srWindow.Right - info.srWindow.Left + 1;
	const SHORT bottom_row = info.srWindow.Bottom;
	const COORD startPos = {
		.X = 0, .Y = bottom_row 
	};

	DWORD written = 0;

	FillConsoleOutputCharacter(handle, ' ',window_width, startPos, &written);
	FillConsoleOutputAttribute(handle, info.wAttributes, window_width, startPos, &written);
}

void CommandSink::redraw_prompt() {
    std::lock_guard guard(console_lock_);

    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO info{};
    GetConsoleScreenBufferInfo(handle, &info);

    const SHORT width = info.dwSize.X;
    std::string_view subcmd(command_);
    const auto clamp = min(width - prompt_.size() - 1, command_.size());
    std::string full = prompt_ + std::string(subcmd.substr(0, clamp));

	boost::container::small_vector<CHAR_INFO, reserve_buf_size> buffer(width);

    for(SHORT i = 0; i < width; ++i) {
        buffer[i].Char.AsciiChar = ' ';
        buffer[i].Attributes = info.wAttributes;
    }

    for(size_t i = 0; i < full.size() && i < width; ++i) {
        buffer[i].Char.AsciiChar = full[i];
    }

	if(colour_) {
		for(size_t i = 0; i < prompt_.size() && i < width; ++i) {
			buffer[i].Attributes = prompt_colour;
		}
	}

    const SHORT bottom = info.dwSize.Y - 1;

    SMALL_RECT region {
		.Left = 0,
		.Top = bottom,
		.Right = gsl::narrow_cast<SHORT>(width - 1),
		.Bottom = bottom
    };

    const COORD buf_size{ 
		.X = width, 
		.Y = 1
	};

    const COORD buf_coord {};

	WriteConsoleOutput(handle, buffer.data(), buf_size, buf_coord, &region);

    COORD cursor{
		.X = gsl::narrow_cast<SHORT>(prompt_.size() + clamp),
        .Y = bottom
    };

    SetConsoleCursorPosition(handle, cursor);
}

void CommandSink::dispatch_command() {
	boost::trim(command_);
	
	// if the string was just whitespace
	if(command_.empty()) {
		redraw_prompt();
		return;
	}

	if(!invoke_handler(command_)) {
		write_buffer("Unable to execute command, no command handler has been registered", true);
	}

	cmd_history_.emplace_back(command_);

	if(cmd_history_.size() > history_size) {
		cmd_history_.pop_front();
	}

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
	redraw_prompt();
}

void CommandSink::delete_character(const bool current) {
	auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info{};
	GetConsoleScreenBufferInfo(handle, &info);

	auto rel_pos = info.dwCursorPosition.X - prompt_.size();

	if(current) {
		command_.erase(rel_pos, 1);
	} else {
		if(rel_pos) {
			command_.erase(--rel_pos, 1);
		}
	}

	redraw_prompt();
	info.dwCursorPosition.X = gsl::narrow_cast<SHORT>(prompt_.size() + rel_pos);
	SetConsoleCursorPosition(handle, info.dwCursorPosition);
}

void CommandSink::buffer_scroll(CursorDirection dir) {
	auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info{};
	GetConsoleScreenBufferInfo(handle, &info);

	if(info.dwCursorPosition.X < prompt_.size()) {
		return;
	}

	auto rel_pos = info.dwCursorPosition.X - prompt_.size();

	if(dir == CursorDirection::back) {
		if(rel_pos) {
			--rel_pos;
		}
	} else if(dir == CursorDirection::forward) {
		++rel_pos;
	}

	// clamp cursor range
	if(rel_pos > command_.size()) {
		rel_pos = command_.size();
	}

	// back to absolute position
	info.dwCursorPosition.X = gsl::narrow_cast<SHORT>(prompt_.size() + rel_pos);
	SetConsoleCursorPosition(handle, info.dwCursorPosition);
}

void CommandSink::insert_character(char ch) {
	auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info{};
	GetConsoleScreenBufferInfo(handle, &info);

	auto rel_pos = info.dwCursorPosition.X - prompt_.size();

	if(rel_pos != command_.size()) {
		command_.insert(rel_pos, &ch, 1);
	} else {
		command_.push_back(ch);
	}

	++rel_pos;

	redraw_prompt();
	info.dwCursorPosition.X = gsl::narrow_cast<SHORT>(prompt_.size() + rel_pos);
	SetConsoleCursorPosition(handle, info.dwCursorPosition);
}

void CommandSink::read_console_input() {
	auto handle = GetStdHandle(STD_INPUT_HANDLE);

	while(!stopped_) {
		std::array<INPUT_RECORD, 32> events{};
		DWORD event_count = 0;
		ReadConsoleInput(handle, events.data(), events.size(), &event_count);
		
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

std::string CommandSink::truncate_description(std::string_view description) {
	constexpr std::string_view ellipsis { "..." };
	static_assert(table_desc_cols >= ellipsis.size());
	const auto ellipsis_space = table_desc_cols - ellipsis.size();
	return std::string(description.substr(0, ellipsis_space)) + "...";
}

void CommandSink::print_command_table(std::span<const commands::Suggestions::Record> matches) {
	boost::container::small_vector<char, max_buf_size> buffer;
	StreamBuffer stream_buffer(buffer);
	std::ostream stream(&stream_buffer);

	bprinter::TablePrinter printer(&stream);
	printer.AddColumn("Command", table_name_cols);
	printer.AddColumn("Description", table_desc_cols);
	printer.PrintHeader();

	for(const auto& match : matches) {
		printer << match.name;

		if(match.desc.size() > table_desc_cols) {
			printer << truncate_description(match.desc);
		} else {
			printer << match.desc;
		}
	}

	printer.PrintFooter();
	write_buffer(buffer, true);
}

void CommandSink::cursor_reposition(const CursorPosition position) {
	auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info{};
	GetConsoleScreenBufferInfo(handle, &info);

	if(position == CursorPosition::start) {
		info.dwCursorPosition.X = gsl::narrow_cast<SHORT>(prompt_.size());
	} else if(position == CursorPosition::end) {
		info.dwCursorPosition.X = gsl::narrow_cast<SHORT>(prompt_.size() + command_.size());
	}

	SetConsoleCursorPosition(handle, info.dwCursorPosition);
}

static std::wstring utf8_to_utf16(std::string_view str) {
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

	auto wlen = MultiByteToWideChar(
		CP_UTF8, 0,
		reinterpret_cast<const char*>(buffer.data()),
		gsl::narrow_cast<int>(buffer.size()),
		nullptr, 0
	);

	if(wlen <= 0) {
		return;
	}

	boost::container::small_vector<wchar_t, reserve_buf_size> wbuf(wlen);

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

		boost::container::small_vector<CHAR_INFO, reserve_buf_size> row(width);

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

void CommandSink::register_handler(CommandHandler handler) {
	std::lock_guard lock(handler_lock_);
	handler_ = std::move(handler);
}

void CommandSink::register_autocomplete(Autocomplete handler) {
	std::lock_guard lock(handler_lock_);
	autocomplete_ = std::move(handler);
}

bool CommandSink::invoke_handler(std::string_view command) {
	if(handler_) {
		handler_(command_);
		return true;
	}
	
	return false;
}

} // log, ember