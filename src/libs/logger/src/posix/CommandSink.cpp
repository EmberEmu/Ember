/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <logger/Exception.h>
#include <logger/Utility.h>
#include <logger/detail/StreamBuffer.h>
#include <logger/posix/CommandSink.h>
#include <logger/detail/PlatformColour.h>
#include <shared/utility/Truncate.h>
#include <bprinter/table_printer.h>
#include <boost/algorithm/string/trim.hpp>
#include <boost/container/small_vector.hpp>
#include <array>
#include <algorithm>
#include <format>
#include <ostream>
#include <stdexcept>
#include <cctype>
#include <cstdio>

namespace ember::log::posix {

constexpr std::string_view ansi_reset = "\x1b[0m"; // clears all attributes
constexpr std::string_view ansi_erase = "\033[K";  // erase to end of line

CommandSink::CommandSink(Severity severity, Filter filter, std::string prompt, Theme theme, bool no_bg)
	: log::CommandSink(std::string(name()), severity, filter)
	, prompt_(std::move(prompt))
	, timestamp_fmt_(default_timestamp_fmt)
	, colour_(false)
	, timestamp_(false)
	, max_cols_(table_max_cols)
	, theme_(std::move(theme))
	, no_bg_(no_bg) {
	const auto [rows, _] = terminal_.get_size();
	terminal_.set_log_region(rows);

	input_reader_ = std::jthread([this](auto stop_token) {
		redraw_prompt();
		read_console_input(stop_token);
	});
}

CommandSink::~CommandSink() {
	stop();

	if(input_reader_.joinable()) {
		input_reader_.join();
	}
}

void CommandSink::stop() {
	input_reader_.request_stop();
	terminal_.wake();
}

void CommandSink::batch_write(const std::span<std::tuple<RecordDetail, std::string_view, std::string>>& records) {
	if(!colour_) [[unlikely]] {
		do_batch_write(records);
	} else { // we can't do batch output if we need to colour each individual log record - todo, we can in this sink, actually
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

	[[indeterminate]] std::array<char, timestamp_buf_size> ts_buffer;
	std::size_t ts_size = 0;

	if(timestamp_) {
		ts_size = generate_timestamp(ts_buffer);
	}

	const std::span<char> timestamp(ts_buffer.data(), ts_size);

	std::lock_guard guard(console_lock_);
	out_buf_.reserve(size + (10 * records.size()));

	for(auto&& [detail, prefix, data] : records) {
		if(sink_sev <= detail.severity && !(sink_filter & detail.type)) {
			std::string_view severity = severity_string(detail.severity);

			// todo, size_t literal on bump
			const auto padding = !prefix.empty()?
				std::max<std::size_t>(1u, prefix_pad_.size() > prefix.size()?
					prefix_pad_.size() - prefix.size() : 0u) : 0u;

			const auto cur_sz = out_buf_.size();

			const auto req_buf_size
				= cur_sz
				+ prefix.size()
				+ padding
				+ timestamp.size()
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
	}

	write_buffer(out_buf_, false);
	redraw_prompt();
	out_buf_.clear();

	if(out_buf_.capacity() > max_buf_size) [[unlikely]] {
		out_buf_.shrink_to_fit();
	}
}

void CommandSink::write(Severity severity, Filter type,	std::string_view prefix, std::string_view record, bool flush) {
	if(this->severity() > severity || (this->filter() & type)) {
		return;
	}

	[[indeterminate]] std::array<char, timestamp_buf_size> ts_buffer;
	std::size_t ts_size = 0;

	if(timestamp_) {
		ts_size = generate_timestamp(ts_buffer);
	}

	const std::span<char> timestamp(ts_buffer.data(), ts_size);
	const std::string_view sevsv = severity_string(severity);
	const std::string_view recordsv = record;

	// todo, size_t literal on bump
	const auto padding = !prefix.empty()?
		std::max<std::size_t>(1u, prefix_pad_.size() > prefix.size()?
			prefix_pad_.size() - prefix.size() : 0u) : 0u;

	// hack, need to make newline handling more consistent rather than... this
	if(record.ends_with('\n')) {
		record.remove_suffix(1);
	}

	// calculate the precise size of the buffer in advance to avoid reallocations
	std::size_t buff_req_size
		= timestamp.size()
		+ prefix.size()
		+ padding
		+ record.size()
		+ sevsv.size();

	std::lock_guard guard(console_lock_);

	const auto prefix_style = theme_colour(theme_, severity, StyleSegment::prefix);
	const auto tag_style = theme_colour(theme_, severity, StyleSegment::tag);
	const auto text_style = theme_colour(theme_, severity, StyleSegment::text);

	if(colour_) {
		if(!no_bg_) {
			buff_req_size += theme_.bg.size();
		}

		if(timestamp_) {
			buff_req_size += theme_.timestamp.size();
		}

		buff_req_size += prefix_style.size();
		buff_req_size += tag_style.size();
		buff_req_size += text_style.size();
		buff_req_size += ansi_erase.size();
		buff_req_size += theme_.reset.size();

		if(!no_bg_) {
			buff_req_size += theme_.bg.size();
		} else {
			buff_req_size += ansi_reset.size();
		}
	}

	out_buf_.resize(buff_req_size, boost::container::default_init);

	std::size_t offset = 0;

	const auto append = [&](auto&& fragment) {
		if(fragment.size() > out_buf_.size() - offset) {
			throw std::runtime_error("output buffer size != offset, bad calculation");
		}

		std::copy(fragment.begin(), fragment.end(), out_buf_.begin() + offset);
		offset += fragment.size();
	};

	// generate output
	if(colour_) [[likely]] {
		if(!no_bg_) {
			append(theme_.bg);
		}

		if(timestamp_) {
			append(theme_.timestamp);
			append(timestamp);
		}

		append(prefix_style);
		append(prefix);
		append(std::string_view(prefix_pad_.data(), padding));
		append(tag_style);
		append(sevsv);
		append(text_style);
		append(record);
		append(ansi_erase);
		append(theme_.reset);

		if(!no_bg_) {
			append(theme_.bg);
		} else {
			append(ansi_reset);
		}
	} else {
		append(timestamp);
		append(sevsv);
		append(record);
	}

	if(offset != buff_req_size) {
		throw std::runtime_error("output buffer size calculation error");
	}

	// output
	write_buffer(out_buf_, false);
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


void CommandSink::set_max_table_cols(unsigned int cols) {
	max_cols_ = std::max(cols, table_min_cols);
}

void CommandSink::clear_line() {
	std::lock_guard guard(console_lock_);

	const auto [rows, _] = terminal_.get_size();
	terminal_.move_to(rows, 1);
	terminal_.clear_line();
}

std::string_view CommandSink::suggestion_substring() {
	if(const auto pos = suggested_.find(command_); pos != std::string::npos) {
		return std::string_view(suggested_).substr(pos + command_.length());
	}

	return {};
}

void CommandSink::redraw_prompt() {
   std::lock_guard guard(console_lock_);

    const auto [rows, cols] = terminal_.get_size();
	
    const auto available = cols > prompt_.size() + 1?
		static_cast<std::size_t>(cols) - prompt_.size() - 1 : 0u; // todo, size_t literal

    const auto visible_command = command_.substr(0, available);
    const auto suggestion = suggestion_substring();

    const auto visible_suggestion = suggestion.substr(0, available > visible_command.size()?
		available - visible_command.size() : 0
    );

    const auto visible_cursor = std::min(cursor_pos_, command_.size());

    const auto cursor_col = std::min(
		prompt_.size() + visible_cursor + 1, static_cast<std::size_t>(cols)
	);

	std::size_t offset = 0;
	[[indeterminate]] boost::container::small_vector<char, max_buf_size> buffer(
		max_buf_size, boost::container::default_init
	);

	const auto append = [&](const std::string_view fragment) {
		const auto required_size = offset + fragment.size();

		if(fragment.size() > buffer.size() - offset) {
			buffer.resize(
				std::max(required_size, buffer.size() * 2),
				boost::container::default_init
			);
		}

		std::copy(fragment.begin(), fragment.end(), buffer.begin() + offset);
		offset += fragment.size();
	};

    append("\x1b[?25l"); // hide cursor
	append("\x1b[");
	append(std::to_string(rows));
	append(";1H"); // move
    append("\x1b[2K"); // erase line

    if(colour_) {
        append(theme_.prompt);
    }

	append(prompt_);

    if(colour_) {
		append(theme_.input);
    }

	append(visible_command);

    if(colour_) {
		append(theme_.suggestion);
    }

	append(visible_suggestion);
	append(theme_.reset);
	append("\x1b[");
	append(std::to_string(rows));
	append(";");
	append(std::to_string(cursor_col));
	append("H");
	append("\x1b[?25h"); // restore cursor

	const std::string_view output(buffer.data(), offset);
    terminal_.write(output);
}

void CommandSink::insert_history(const std::string& command) {
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
	cursor_pos_ = command_.size();
	suggested_.clear();

	if(command_.empty()) {
		redraw_prompt();
		return;
	}

	if(!invoke_handler(command_)) {
		constexpr std::string_view message =
			"Unable to execute command, no command handler has been registered\n";

		if(colour_) {
			std::lock_guard guard(console_lock_);
			terminal_.write(theme_.text.conerr);
			write_buffer(std::span(message.data(), message.size()), false);
			terminal_.write(theme_.reset);
		} else {
			write_buffer(std::span(message.data(), message.size()), false);
		}
	}

	insert_history(command_);
	history_idx_ = cmd_history_.size();

	command_.clear();
	cursor_pos_ = 0;

	redraw_prompt();
}

void CommandSink::history_scroll(ScrollDirection dir) {
	if(cmd_history_.empty()) {
		return;
	}

	if(dir == ScrollDirection::up) {
		if(history_idx_ > 0) {
			--history_idx_;
		}
	} else if(history_idx_ < cmd_history_.size()) {
		++history_idx_;
	}

	if(history_idx_ == cmd_history_.size()) {
		command_.clear();
	} else {
		command_ = cmd_history_[history_idx_];
	}

	cursor_pos_ = command_.size();

	update_suggestion();
	redraw_prompt();
}

void CommandSink::delete_character(bool after) {
	if(after) {
		if(cursor_pos_ < command_.size()) {
			command_.erase(cursor_pos_, 1);
		}
	} else if(cursor_pos_ > 0) {
		command_.erase(--cursor_pos_, 1);
	}

	update_suggestion();
	redraw_prompt();
}

void CommandSink::buffer_scroll(CursorDirection dir) {
	if(dir == CursorDirection::back) {
		if(cursor_pos_ > 0) {
			--cursor_pos_;
		}
	} else if(cursor_pos_ < command_.size()) {
		++cursor_pos_;
	}

	redraw_prompt();
}

void CommandSink::insert_character(char ch) {
	if(command_.size() >= max_command_length) {
		return;
	}

	command_.insert(cursor_pos_, 1, ch);
	++cursor_pos_;

	const auto suggestion = suggestion_substring();

	// prevent calling the handler for every key stroke if we
	// know the result wouldn't change
	if((suggestion.empty() && !suggested_.empty()) ||
		command_.size() == 1 ||
		cursor_pos_ != command_.size()) {
		update_suggestion();
	}

	redraw_prompt();
}

void CommandSink::read_console_input(std::stop_token stop) {
	using Key = TerminalCtl::Key;

	while(!stop.stop_requested()) {
		const auto event = terminal_.read_key();

		if(stop.stop_requested()) {
			break;
		}

		std::lock_guard guard(console_lock_);

		switch (event.key) {
			case Key::Character:
				if(std::isprint(static_cast<unsigned char>(event.ch))) {
					insert_character(static_cast<char>(event.ch));
				}
				break;
			case Key::Enter:
				dispatch_command();
				break;
			case Key::Tab:
				autocomplete();
				break;
			case Key::Backspace:
				delete_character(false);
				break;
			case Key::Delete:
				delete_character(true);
				break;
			case Key::Up:
				history_scroll(ScrollDirection::up);
				break;
			case Key::Down:
				history_scroll(ScrollDirection::down);
				break;
			case Key::Left:
				buffer_scroll(CursorDirection::back);
				break;
			case Key::Right:
				buffer_scroll(CursorDirection::forward);
				break;
			case Key::Home:
				cursor_reposition(CursorPosition::start);
				break;
			case Key::End:
				cursor_reposition(CursorPosition::end);
				break;
			case Key::Interrupted:
				break;
			case Key::Escape:
				[[fallthrough]];
			case Key::Other:
				break;
			case Key::EndOfInput:
				return;
		}
	}
}

void CommandSink::autocomplete() {
	Autocomplete handler;

	{
		std::lock_guard guard(handler_lock_);
		handler = autocomplete_;
	}

	if(!handler) {
		return;
	}

	const auto result = handler(command_);

	if(result.records.empty()) {
		return;
	}

	std::lock_guard guard(console_lock_);

	command_ = result.substring;
	cursor_pos_ = command_.size();

	print_command_table(result.records);

	update_suggestion();
	redraw_prompt();
}

void CommandSink::print_command_table(std::span<const commands::Suggestions::Record> matches) {
	boost::container::small_vector<char, max_buf_size> buffer;
	detail::StreamBuffer stream_buffer(buffer);
	std::ostream stream(&stream_buffer);

	const auto [_, cols] = terminal_.get_size();

	const auto columns = std::max(
		std::min(cols, max_cols_) - table_padding, table_min_cols - table_padding
	);

	const auto name_cols = std::max(columns / 3, table_name_min_cols);
	const auto desc_cols = std::max(columns - name_cols, table_desc_min_cols);

	bprinter::TablePrinter printer(&stream);

	printer.AddColumn("Command", name_cols);
	printer.AddColumn("Description", desc_cols);
	printer.PrintHeader();

	for(const auto& match : matches) {
		utility::write_pretty_truncate(printer, match.name, name_cols);
		utility::write_pretty_truncate(printer, match.desc, desc_cols);
	}

	printer.PrintFooter();
	write_buffer(buffer, false);
}

void CommandSink::cursor_reposition(CursorPosition position) {
	cursor_pos_ = position == CursorPosition::start? 0 : command_.size();
	redraw_prompt();
}

void CommandSink::clear_console() {
	std::lock_guard guard(console_lock_);

	terminal_.clear_screen();
	log_line_used_ = false;

	const auto [rows, _] = terminal_.get_size();
	terminal_.set_log_region(rows);

	redraw_prompt();
}

void CommandSink::write_buffer(std::span<const char> buffer, bool redraw) {
	std::lock_guard guard(console_lock_);

    if(buffer.empty()) {
        if (redraw) {
            redraw_prompt();
        }

        return;
    }

    const auto [rows, _] = terminal_.get_size();
    terminal_.set_log_region(static_cast<int>(rows));

    // scroll the log region up
    if(log_line_used_) {
        terminal_.write("\x1b[S");
    }

    terminal_.move_to(std::max(1u, rows - 1), 1);
    terminal_.clear_line();

    auto text = std::string_view(buffer.data(), buffer.size());

    if(text.ends_with('\n')) {
        text.remove_suffix(1);
    }

    terminal_.write(text);
	log_line_used_ = true;

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

void CommandSink::register_autocomplete(Autocomplete handler) {
	std::lock_guard guard(handler_lock_);
	autocomplete_ = std::move(handler);
}

void CommandSink::register_handler(CommandHandler handler) {
	std::lock_guard guard(handler_lock_);
	handler_ = std::move(handler);
}

void CommandSink::register_suggestion(Suggestion handler) {
	std::lock_guard guard(handler_lock_);
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

void CommandSink::set_theme(Theme theme) {
	std::lock_guard guard(console_lock_);
	theme_ = std::move(theme);

	if(colour_ && !no_bg_) {
		terminal_.write(theme_.bg);
	}
}

} // posix, log, ember
