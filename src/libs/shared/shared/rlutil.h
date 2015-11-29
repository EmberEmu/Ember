#pragma once
/**
 * File: rlutil.h
 *
 * About: Description
 * This file provides some useful utilities for console mode
 * roguelike game development with C and C++. It is aimed to
 * be cross-platform (at least Windows and Linux).
 *
 * About: Copyright
 * (C) 2010 Tapio Vierros
 *
 * About: Licensing
 * See <License>
 */

#include <iostream>
#include <string>
#include <sstream>

/// Define: RLUTIL_USE_ANSI
/// Define this to use ANSI escape sequences also on Windows
/// (defaults to using WinAPI instead).
#if 0
#define RLUTIL_USE_ANSI
#endif

#ifdef _WIN32
	#include <windows.h>  // for WinAPI and Sleep()
	#define _NO_OLDNAMES  // for MinGW compatibility
#endif

namespace rlutil {

inline void RLUTIL_PRINT(const std::string& st) {
	std::cout << st; 
}

enum {
	BLACK,
	BLUE,
	GREEN,
	CYAN,
	RED,
	MAGENTA,
	BROWN,
	GREY,
	DARKGREY,
	LIGHTBLUE,
	LIGHTGREEN,
	LIGHTCYAN,
	LIGHTRED,
	LIGHTMAGENTA,
	YELLOW,
	WHITE
};

const std::string ANSI_RESET = "\033[0m";
const std::string ANSI_CLS = "\033[2J";
const std::string ANSI_BLACK = "\033[22;30m";
const std::string ANSI_RED = "\033[22;31m";
const std::string ANSI_GREEN = "\033[22;32m";
const std::string ANSI_BROWN = "\033[22;33m";
const std::string ANSI_BLUE = "\033[22;34m";
const std::string ANSI_MAGENTA = "\033[22;35m";
const std::string ANSI_CYAN = "\033[22;36m";
const std::string ANSI_GREY = "\033[22;37m";
const std::string ANSI_DARKGREY = "\033[01;30m";
const std::string ANSI_LIGHTRED = "\033[01;31m";
const std::string ANSI_LIGHTGREEN = "\033[01;32m";
const std::string ANSI_YELLOW = "\033[01;33m";
const std::string ANSI_LIGHTBLUE = "\033[01;34m";
const std::string ANSI_LIGHTMAGENTA = "\033[01;35m";
const std::string ANSI_LIGHTCYAN = "\033[01;36m";
const std::string ANSI_WHITE = "\033[01;37m";

/// Function: getANSIColor
/// Return ANSI color escape sequence for specified number 0-15.
///
/// See <Color Codes>
inline std::string getANSIColor(int c) {
	switch (c) {
		case 0 : return ANSI_BLACK;
		case 1 : return ANSI_BLUE; // non-ANSI
		case 2 : return ANSI_GREEN;
		case 3 : return ANSI_CYAN; // non-ANSI
		case 4 : return ANSI_RED; // non-ANSI
		case 5 : return ANSI_MAGENTA;
		case 6 : return ANSI_BROWN;
		case 7 : return ANSI_GREY;
		case 8 : return ANSI_DARKGREY;
		case 9 : return ANSI_LIGHTBLUE; // non-ANSI
		case 10: return ANSI_LIGHTGREEN;
		case 11: return ANSI_LIGHTCYAN; // non-ANSI;
		case 12: return ANSI_LIGHTRED; // non-ANSI;
		case 13: return ANSI_LIGHTMAGENTA;
		case 14: return ANSI_YELLOW; // non-ANSI
		case 15: return ANSI_WHITE;
		default: return "";
	}
}

#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
//WORD origColor;
#endif

inline void saveColor() {
	#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
	const HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO buffer;
	GetConsoleScreenBufferInfo(stdout_handle, &buffer);
	//origColor = buffer.wAttributes;
	#else
		// todo
	#endif
}

/// Function: setColor
/// Change color specified by number (Windows / QBasic colors).
///
/// See <Color Codes>
inline void setColor(int c) {
#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, (WORD)c);
#else
	rlutil::RLUTIL_PRINT(rlutil::getANSIColor(c));
#endif
}

/// Function: resetColor
/// Reset color to ??
inline void resetColor() {
#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	//SetConsoleTextAttribute(hConsole, origColor);
#else
	rlutil::RLUTIL_PRINT(rlutil::ANSI_RESET);
#endif
}


} // namespace rlutil