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
#include <bprinter/table_printer.h>
#include <gsl/narrow>
#include <boost/algorithm/string.hpp>
#include <Windows.h>
#include <algorithm>
#include <array>
#include <iostream>
#include <iterator>
#include <string_view>
#include <string>
#include <cstdio>
#include <cstring>

namespace ember::log {

CommandSink::CommandSink(Severity severity, Filter filter, std::string prompt)
	: Sink(severity, filter, "CommandSink"),
	  prompt_(std::move(prompt)),
	  colour_(false),
	  stopped_(false),
	  history_idx_(0) {
	redraw_prompt();

	event_handler_ = std::jthread([&]() {
		read_console_input();
	});
}

CommandSink::~CommandSink() {
	stop();
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

	out_buf_.clear();
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

	write_buffer();

	if(out_buf_.capacity() > max_buf_size) [[unlikely]] {
		out_buf_.clear();
		out_buf_.shrink_to_fit();
	}
}

void CommandSink::write(Severity severity, Filter type, std::span<const char> record, bool flush) {
	if(this->severity() > severity || (this->filter() & type)) {
		return;
	}

	std::string_view sevsv = detail::severity_string(severity);

	out_buf_.clear();
	out_buf_.resize(prefix_.size() + record.size() + sevsv.size(), boost::container::default_init);

	std::size_t offset = 0;
	std::memcpy(out_buf_.data(), prefix_.data(), prefix_.size());
	offset += prefix_.size();
	std::memcpy(out_buf_.data() + offset, sevsv.data(), sevsv.size());
	offset += sevsv.size();
	std::memcpy(out_buf_.data() + offset, record.data(), record.size());

	if(colour_) [[likely]] {
		std::lock_guard guard(colour_lock);
		util::ConsoleColour concol(severity_colour(severity));
		write_buffer(false);
	} else {
		write_buffer(false);
	}

	redraw_prompt();

	if(flush) {
		if(std::fflush(stdout) != 0) {
			throw exception("Unable to flush log record to console");
		}
	}

	if(out_buf_.capacity() > max_buf_size) [[unlikely]] {
		out_buf_.clear();
		out_buf_.shrink_to_fit();
	}
}

util::Colour CommandSink::severity_colour(Severity severity) {
	switch(severity) {
		case Severity::CONSOLE:
			return util::Colour::WHITE_ON_CYAN_BG;
		case Severity::CONSOLE_ERROR:
			return util::Colour::WHITE_ON_RED_BG;
		case Severity::FATAL:
			[[fallthrough]];
		case Severity::ERROR_:
			[[fallthrough]];
		case Severity::WARN:
			return util::Colour::LIGHT_RED;
		case Severity::INFO:
			return util::Colour::WHITE;
		case Severity::DEBUG:
			return util::Colour::LIGHT_CYAN;
		case Severity::TRACE:
			return util::Colour::DARK_GREY;
		case Severity::DISABLED:
			[[fallthrough]];
		default:
			return util::Colour::DEFAULT;
	}
}

void CommandSink::clear_line() {
	auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo(handle, &info);
	info.dwCursorPosition.X = 0;

	SetConsoleCursorPosition(handle, info.dwCursorPosition);
	std::string clear(info.dwSize.X - 1, ' ');
	std::cout << clear;
	SetConsoleCursorPosition(handle, info.dwCursorPosition);
}

void CommandSink::redraw_prompt() {
	std::lock_guard guard(console_lock_);

	auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info{};
	GetConsoleScreenBufferInfo(handle, &info);

	const COORD cursor_pos {
		.X = 0,
		.Y = info.dwSize.Y - 1
	};

	clear_line();
	SetConsoleCursorPosition(handle, cursor_pos);

	// restrict displayed command length to keep the console output tidy
	const auto clamp = min(info.dwSize.X - prompt_.size() - 1, command_.size());
	std::string_view subcmd(command_);
	std::cout << prompt_ << subcmd.substr(0, clamp);
}

void CommandSink::dispatch_command() {
	boost::trim(command_);
	
	// if the string was just whitespace
	if(command_.empty()) {
		redraw_prompt();
		return;
	}

	if(handler_) {
		handler_(command_);
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
		
		for(const auto& e : std::span(events)) {
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

	if(result.commands.empty()) {
		return;
	}
	
	std::lock_guard guard(console_lock_);
	command_ = result.command;
	clear_line();
	print_command_table(result.commands);
	redraw_prompt();
}

void CommandSink::print_command_table(std::span<const std::string> commands) const {
	bprinter::TablePrinter printer(&std::cout);
	printer.AddColumn("Command", 20);
	printer.AddColumn("Description", 50);
	printer.PrintHeader();

	std::string desc_tmp = "Lorem ipsum dolor sit amet consectetur adipiscing elit quisque";

	for(const auto& command : commands) {
		printer << command;

		if(desc_tmp.size() > 50) {
			const std::string truncated = desc_tmp.substr(0, 45) + "...";
			printer << truncated;
		} else {
			printer << desc_tmp;
		}
	}

	printer.PrintFooter();
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

void CommandSink::write_buffer(bool redraw) {
	std::lock_guard guard(console_lock_);

	clear_line();
	std::fwrite(out_buf_.data(), out_buf_.size(), 1, stdout);

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

} // log, ember