/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/posix/Keys.h>
#include <string_view>
#include <system_error>
#include <utility>
#include <cerrno>
#include <cstdio>
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#else
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif

namespace ember::log::posix {

class TerminalControl {
	constexpr static auto default_rows = 24u;
	constexpr static auto default_cols = 80u;

	[[noreturn]] static void fail(const char* operation);

#ifdef _WIN32
	KeyModifier get_modifiers(const DWORD state);
#else
	int read_byte(unsigned char& ch, int timeout_ms = -1);
	KeyEvent decode_byte(unsigned char ch);
	KeyEvent read_escape_sequence();
#endif

	void restore();

#ifdef _WIN32
	HANDLE input_handle_ = INVALID_HANDLE_VALUE;
	HANDLE output_handle_ = INVALID_HANDLE_VALUE;
	HANDLE wake_event_ = nullptr;

	DWORD original_input_mode_ = 0;
	DWORD original_output_mode_ = 0;
#else
	termios original_{};
	int wake_read_ = -1;
	int wake_write_ = -1;
#endif
	bool active_ = false;

public:
	TerminalControl();
	~TerminalControl();

    TerminalControl(const TerminalControl&) = delete;
    TerminalControl& operator=(const TerminalControl&) = delete;

	void write(std::string_view text);
	void move_to(int row, int col);
	void clear_line();
	void clear_screen();
	std::pair<unsigned int, unsigned int> get_size();
	void set_log_region(int rows);
	void wake();
	KeyEvent read_key();
};

} // posix, log, ember