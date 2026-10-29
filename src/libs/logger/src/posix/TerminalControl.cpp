/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <logger/posix/TerminalControl.h>
#include <algorithm>
#include <array>
#include <string>

namespace ember::log::posix {

#ifdef _WIN32

TerminalControl::TerminalControl() {
	input_handle_ = GetStdHandle(STD_INPUT_HANDLE);
	output_handle_ = GetStdHandle(STD_OUTPUT_HANDLE);

	if(input_handle_ == INVALID_HANDLE_VALUE ||
		output_handle_ == INVALID_HANDLE_VALUE) {
		throw std::runtime_error("Invalid console handles");
	}

	if(!GetConsoleMode(input_handle_, &original_input_mode_)) {
		throw std::system_error(
			static_cast<int>(GetLastError()),
			std::system_category(),
			"GetConsoleMode(input)"
		);
	}

	if(!GetConsoleMode(output_handle_, &original_output_mode_)) {
		throw std::system_error(
			static_cast<int>(GetLastError()),
			std::system_category(),
			"GetConsoleMode(output)"
		);
	}

	const auto output_mode = original_output_mode_ | ENABLE_VIRTUAL_TERMINAL_PROCESSING;

	if(!SetConsoleMode(output_handle_, output_mode)) {
		throw std::system_error(
			static_cast<int>(GetLastError()),
			std::system_category(),
			"SetConsoleMode(output)"
		);
	}

	const auto input_mode =
		(original_input_mode_ | ENABLE_EXTENDED_FLAGS) &
		~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT |
		  ENABLE_VIRTUAL_TERMINAL_INPUT);

	if(!SetConsoleMode(input_handle_, input_mode)) {
		const auto error = GetLastError();
		SetConsoleMode(output_handle_, original_output_mode_);

		throw std::system_error(
			static_cast<int>(error),
			std::system_category(),
			"SetConsoleMode(input)"
		);
	}

	wake_event_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);

	if(!wake_event_) {
		const auto error = GetLastError();

		SetConsoleMode(input_handle_, original_input_mode_);
		SetConsoleMode(output_handle_, original_output_mode_);

		throw std::system_error(
			static_cast<int>(error),
			std::system_category(),
			"CreateEvent"
		);
	}

	active_ = true;
	write("\x1b[0m"); // reset formatting
}

#else

TerminalControl::TerminalControl() {
	if(!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
		throw std::runtime_error("CommandSink requires a TTY");
	}

	if(tcgetattr(STDIN_FILENO, &original_) == -1) {
		fail("tcgetattr");
	}

	std::array<int, 2> fds;

	if(pipe(fds.data()) == -1) {
		fail("pipe");
	}

	wake_read_ = fds[0];
	wake_write_ = fds[1];

	for(const auto& fd : fds) {
		const int fd_flags = fcntl(fd, F_GETFD);

		if(fd_flags == -1 || fcntl(fd, F_SETFD, fd_flags | FD_CLOEXEC) == -1) {
			fail("fcntl");
		}

		const int status_flags = fcntl(fd, F_GETFL);

		if(status_flags == -1 || fcntl(fd, F_SETFL, status_flags | O_NONBLOCK) == -1) {
			fail("fcntl");
		}
	}

	termios raw = original_;
	raw.c_lflag &= ~(ICANON | ECHO);
	raw.c_iflag &= ~(IXON | ICRNL);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;

	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
		fail("tcsetattr");
	}

	active_ = true;
	write("\x1b[0m"); // reset, todo etc :)
}

#endif

TerminalControl::~TerminalControl() {
	restore();

#ifdef _WIN32
	if(wake_event_) {
		CloseHandle(wake_event_);
	}
#else

	if(wake_read_ != -1) {
		close(wake_read_);
	}

	if(wake_write_ != -1) {
		close(wake_write_);
	}
#endif
}

#ifdef _WIN32

void TerminalControl::write(std::string_view text) {
	auto data = text.data();
	auto remaining = text.size();

	while(remaining > 0) {
		const auto chunk = static_cast<DWORD>(
			std::min<std::size_t>(remaining, std::numeric_limits<DWORD>::max())
		);

		DWORD written = 0;

		if(!WriteFile(output_handle_, data, chunk, &written, nullptr)) {
			throw std::system_error(
				static_cast<int>(GetLastError()),
				std::system_category(),
				"terminal write"
			);
		}

		if(written == 0) {
			throw std::runtime_error("Terminal write stalled");
		}

		data += written;
		remaining -= written;
	}
}

#else

void TerminalControl::write(std::string_view text) {
	const char* data = text.data();
	std::size_t left = text.size();

	while(left) {
		const auto count = ::write(STDOUT_FILENO, data, left);

		if(count > 0) {
			data += count;
			left -= static_cast<std::size_t>(count);
		} else if(count == -1 && errno == EINTR) {
			continue;
		} else {
			throw std::system_error(errno, std::generic_category(), "terminal write");
		}
	}
}

#endif

void TerminalControl::move_to(int row, int col) {
	write("\x1b[" + std::to_string(std::max(1, row)) + ";" +
		  std::to_string(std::max(1, col)) + "H");
}

void TerminalControl::clear_line() {
	write("\x1b[2K");
}

void TerminalControl::clear_screen() {
	write("\x1b[2J\x1b[H");
}

#ifdef _WIN32

std::pair<unsigned int, unsigned int> TerminalControl::get_size() {
	CONSOLE_SCREEN_BUFFER_INFO info{};

	if(!GetConsoleScreenBufferInfo(output_handle_, &info)) {
		return { default_rows, default_cols };
	}

	const auto rows = info.srWindow.Bottom - info.srWindow.Top + 1;
	const auto cols = info.srWindow.Right - info.srWindow.Left + 1;

	if(rows <= 0 || cols <= 0) {
		return { default_rows, default_cols };
	}

	return {
		static_cast<unsigned int>(rows),
		static_cast<unsigned int>(cols)
	};
}
#else

std::pair<unsigned int, unsigned int> TerminalControl::get_size() {
	winsize ws{};

	if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 && ws.ws_col > 0) {
		return { ws.ws_row, ws.ws_col };
	} else {
		return { default_rows, default_cols };
	}
}

#endif

void TerminalControl::set_log_region(int rows) {
	if(rows >= 2) {
		write("\x1b[1;" + std::to_string(rows - 1) + "r");
	} else {
		write("\x1b[r");
	}
}

#ifdef _WIN32

void TerminalControl::wake() {
	if(wake_event_) {
		SetEvent(wake_event_);
	}
}

#else

void TerminalControl::wake() {
	const char byte = 1;
	auto res = ::write(wake_write_, &byte, 1);
	(void)res; // todo, temp
}

#endif

#ifdef _WIN32

auto TerminalControl::read_key() -> KeyEvent {
	const std::array<HANDLE, 2> handles {
		input_handle_, wake_event_
	};

	for(;;) {
		const auto result = WaitForMultipleObjects(
			handles.size(), handles.data(), FALSE, INFINITE
		);

		if(result == WAIT_OBJECT_0 + 1) {
			ResetEvent(wake_event_);
			return { Key::Interrupted, 0 };
		}

		if(result != WAIT_OBJECT_0) {
			return { Key::EndOfInput, 0 };
		}

		INPUT_RECORD record{};
		DWORD count = 0;

		if(!ReadConsoleInputW(input_handle_, &record, 1, &count)) {
			return { Key::EndOfInput, 0 };
		}

		if(count == 0 || record.EventType != KEY_EVENT) {
			continue;
		}

		const auto& key = record.Event.KeyEvent;

		if(!key.bKeyDown) {
			continue;
		}

		switch (key.wVirtualKeyCode) {
			case VK_RETURN:
				return { Key::Enter, 0 };
			case VK_TAB:
				return { Key::Tab, 0 };
			case VK_ESCAPE:
				return { Key::Escape, 0 };
			case VK_BACK:
				return { Key::Backspace, 0 };
			case VK_DELETE:
				return { Key::Delete, 0 };
			case VK_UP:
				return { Key::Up, 0 };
			case VK_DOWN:
				return { Key::Down, 0 };
			case VK_LEFT:
				return { Key::Left, 0 };
			case VK_RIGHT:
				return { Key::Right, 0 };
			case VK_HOME:
				return { Key::Home, 0 };
			case VK_END:
				return { Key::End, 0 };
			default:
				break;
		}

		const auto ch = key.uChar.UnicodeChar;

		if(ch >= 0x20 && ch <= 0x7e) {
			return {
				Key::Character,
				static_cast<unsigned char>(ch)
			};
		}
	}
}

#else

auto TerminalControl::read_key() -> KeyEvent {
	unsigned char ch{};
	const int first = read_byte(ch);

	if(first <= 0) {
		return { first == 0? Key::Interrupted : Key::EndOfInput, 0 };
	}

	switch(ch) {
		case '\r':
			[[fallthrough]];
		case '\n':
			return { Key::Enter, 0 };
		case '\t':
			return { Key::Tab, 0 };
		case 0x7f:
			[[fallthrough]];
		case 0x08:
			return { Key::Backspace, 0 };
		case 0x1b:
			return read_escape_sequence();
		default:
			if(ch >= 0x20 && ch <= 0x7e) {
				return { Key::Character, ch };
			}

			return { Key::Other, ch };
	}
}

#endif

[[noreturn]] void TerminalControl::fail(const char* operation) {
	throw std::system_error(errno, std::generic_category(), operation);
}

#ifndef _WIN32

// 1 = byte read, 0 = wake pipe, -1 = eof/error.
int TerminalControl::read_byte(unsigned char& ch, int timeout_ms) {
	std::array<pollfd, 2> fds {
		pollfd { STDIN_FILENO, POLLIN, 0 },
		pollfd { wake_read_,   POLLIN, 0 }
	};

	int result = -1;

	do {
		result = poll(fds.data(), fds.size(), timeout_ms);
	} while(result == -1 && errno == EINTR);

	if(result == 0) {
		return 0;
	}

	if(result < 0) {
		return -1;
	}

	if(fds[1].revents & (POLLIN | POLLHUP | POLLERR)) {
		unsigned char discard[64];
		while(::read(wake_read_, discard, sizeof(discard)) > 0);
		return 0;
	}

	if(fds[0].revents & (POLLHUP | POLLERR)) {
		return -1;
	}

	if(!(fds[0].revents & POLLIN)) {
		return 0;
	}

	for(;;) {
		const auto count = ::read(STDIN_FILENO, &ch, 1);

		if(count == 0) {
			return -1;
		} else if(count == 1) {
			return 1;
		}
	}
}

auto TerminalControl::read_escape_sequence() -> KeyEvent {
	unsigned char byte{};

	if(read_byte(byte, 35) != 1) {
		return { Key::Escape, 0 };
	}

	if(byte != '[' && byte != 'O') {
		return { Key::Escape, 0 };
	}

	const bool ss3 = byte == 'O';

	if(read_byte(byte, 35) != 1) {
		return { Key::Escape, 0 };
	}

	if(ss3) {
		switch(byte) {
			case 'B':
				return { Key::Down, 0 };
			case 'A':
				return { Key::Up, 0 };
			case 'C':
				return { Key::Right, 0 };
			case 'D':
				return { Key::Left, 0 };
			case 'H':
				return { Key::Home, 0 };
			case 'F':
				return { Key::End, 0 };
			default:
				return { Key::Other, 0 };
		}
	}

	switch(byte) {
		case 'A':
			return { Key::Up, 0 };
		case 'B':
			return { Key::Down, 0 };
		case 'C':
			return { Key::Right, 0 };
		case 'D':
			return { Key::Left, 0 };
		case 'H':
			return { Key::Home, 0 };
		case 'F':
			return { Key::End, 0 };
		case '1':
			[[fallthrough]];
		case '4':
			[[fallthrough]];
		case '3':
		{
			const unsigned char first = byte;

			if(read_byte(byte, 35) != 1 || byte != '~') {
				return { Key::Other, 0 };
			}

			if(first == '3') {
				return { Key::Delete, 0 };
			}

			if(first == '1') {
				return { Key::Home, 0 };
			}

			return { Key::End, 0 };
		}
		default:
			return { Key::Other, 0 };
	}
}

#endif

#ifdef _WIN32

void TerminalControl::restore() {
	if(!active_) {
		return;
	}

	constexpr std::string_view cleanup =
		"\x1b[0m\x1b[r\x1b[?25h";

	try {
		write(cleanup);
	} catch (...) {
		// Restoration of console modes must still be attempted.
	}

	SetConsoleMode(input_handle_, original_input_mode_);
	SetConsoleMode(output_handle_, original_output_mode_);

	active_ = false;
}

#else

void TerminalControl::restore() {
	if(!active_) {
		return;
	}

	constexpr std::string_view cleanup = "\x1b[0m\x1b[r\x1b[?25h";

	auto data = cleanup.data();
	auto remaining = cleanup.size();

	while(remaining) {
		const auto count = ::write(STDOUT_FILENO, data, remaining);

		if(count > 0) {
			data += count;
			remaining -= static_cast<std::size_t>(count);
		} else if(count == -1 && errno == EINTR) {
			continue;
		} else {
			break;
		}
	}

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_);
	active_ = false;
}

#endif

} // posix, log, ember
