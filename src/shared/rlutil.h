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

/// Namespace forward declarations
namespace rlutil {
	void locate(int x, int y);
}

#ifdef _WIN32
	#include <windows.h>  // for WinAPI and Sleep()
	#define _NO_OLDNAMES  // for MinGW compatibility
	#include <conio.h>    // for getch() and kbhit()
	#define getch _getch
	#define kbhit _kbhit
#else
	#include <cstdio> // for getch()
	#include <termios.h> // for getch() and kbhit()
	#include <unistd.h> // for getch(), kbhit() and (u)sleep()
	#include <sys/ioctl.h> // for getkey()
	#include <sys/types.h> // for kbhit()
	#include <sys/time.h> // for kbhit()

/// Function: getch
/// Get character without waiting for Return to be pressed.
/// Windows has this in conio.h
int getch() {
	// Here be magic.
	struct termios oldt, newt;
	int ch;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	ch = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return ch;
}

/// Function: kbhit
/// Determines if keyboard has been hit.
/// Windows has this in conio.h
int kbhit() {
	// Here be dragons.
	static struct termios oldt, newt;
	int cnt = 0;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag    &= ~(ICANON | ECHO);
	newt.c_iflag     = 0; // input mode
	newt.c_oflag     = 0; // output mode
	newt.c_cc[VMIN]  = 1; // minimum time to wait
	newt.c_cc[VTIME] = 1; // minimum characters to wait for
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	ioctl(0, FIONREAD, &cnt); // Read count
	struct timeval tv;
	tv.tv_sec  = 0;
	tv.tv_usec = 100;
	select(STDIN_FILENO+1, NULL, NULL, NULL, &tv); // A small time delay
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return cnt; // Return number of characters
}
#endif // _WIN32

namespace rlutil {

void RLUTIL_PRINT(const std::string& st) {
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

const int KEY_ESCAPE  = 0;
const int KEY_ENTER   = 1;
const int KEY_SPACE   = 32;

const int KEY_INSERT  = 2;
const int KEY_HOME    = 3;
const int KEY_PGUP    = 4;
const int KEY_DELETE  = 5;
const int KEY_END     = 6;
const int KEY_PGDOWN  = 7;

const int KEY_UP      = 14;
const int KEY_DOWN    = 15;
const int KEY_LEFT    = 16;
const int KEY_RIGHT   = 17;

const int KEY_F1      = 18;
const int KEY_F2      = 19;
const int KEY_F3      = 20;
const int KEY_F4      = 21;
const int KEY_F5      = 22;
const int KEY_F6      = 23;
const int KEY_F7      = 24;
const int KEY_F8      = 25;
const int KEY_F9      = 26;
const int KEY_F10     = 27;
const int KEY_F11     = 28;
const int KEY_F12     = 29;

const int KEY_NUMDEL  = 30;
const int KEY_NUMPAD0 = 31;
const int KEY_NUMPAD1 = 127;
const int KEY_NUMPAD2 = 128;
const int KEY_NUMPAD3 = 129;
const int KEY_NUMPAD4 = 130;
const int KEY_NUMPAD5 = 131;
const int KEY_NUMPAD6 = 132;
const int KEY_NUMPAD7 = 133;
const int KEY_NUMPAD8 = 134;
const int KEY_NUMPAD9 = 135;

/// Function: getkey
/// Reads a key press (blocking) and returns a key code.
///
/// See <Key codes for keyhit()>
///
/// Note:
/// Only Arrows, Esc, Enter and Space are currently working properly.
int getkey() {
	#ifndef _WIN32
	int cnt = kbhit(); // for ANSI escapes processing
	#endif
	int k = getch();
	switch(k) {
		case 0: {
			int kk;
			switch(kk = getch()) {
				case 71: return KEY_NUMPAD7;
				case 72: return KEY_NUMPAD8;
				case 73: return KEY_NUMPAD9;
				case 75: return KEY_NUMPAD4;
				case 77: return KEY_NUMPAD6;
				case 79: return KEY_NUMPAD1;
				case 80: return KEY_NUMPAD4;
				case 81: return KEY_NUMPAD3;
				case 82: return KEY_NUMPAD0;
				case 83: return KEY_NUMDEL;
				default: return kk - 59 + KEY_F1; // Function keys
			}}
		case 224: {
			int kk;
			switch(kk = getch()) {
				case 71: return KEY_HOME;
				case 72: return KEY_UP;
				case 73: return KEY_PGUP;
				case 75: return KEY_LEFT;
				case 77: return KEY_RIGHT;
				case 79: return KEY_END;
				case 80: return KEY_DOWN;
				case 81: return KEY_PGDOWN;
				case 82: return KEY_INSERT;
				case 83: return KEY_DELETE;
				default: return kk - 123 + KEY_F1; // Function keys
			}}
		case 13: return KEY_ENTER;
#ifdef _WIN32
		case 27: return KEY_ESCAPE;
#else // _WIN32
		case 155: // single-character CSI
		case 27: {
			// Process ANSI escape sequences
			if (cnt >= 3 && getch() == '[') {
				switch (k = getch()) {
					case 'A': return KEY_UP;
					case 'B': return KEY_DOWN;
					case 'C': return KEY_RIGHT;
					case 'D': return KEY_LEFT;
				}
			} else {
				return KEY_ESCAPE;
			}
		}
#endif // _WIN32
		default: return k;
	}
}

/// Function: nb_getch
/// Non-blocking getch(). Returns 0 if no key was pressed.
int nb_getch() {
	if (kbhit()) return getch();
	else return 0;
}

/// Function: getANSIColor
/// Return ANSI color escape sequence for specified number 0-15.
///
/// See <Color Codes>
std::string getANSIColor(int c) {
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
WORD origColor;
#endif

void saveColor() {
	#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
	const HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO buffer;
	GetConsoleScreenBufferInfo(stdout_handle, &buffer);
	origColor = buffer.wAttributes;
	#else
		// todo
	#endif
}

/// Function: setColor
/// Change color specified by number (Windows / QBasic colors).
///
/// See <Color Codes>
void setColor(int c) {
#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, (WORD)c);
#else
	rlutil::RLUTIL_PRINT(rlutil::getANSIColor(c));
#endif
}

/// Function: resetColor
/// Reset color to ??
void resetColor() {
#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, origColor);
#else
	rlutil::RLUTIL_PRINT(rlutil::ANSI_RESET);
#endif
}

/// Function: cls
/// Clears screen and moves cursor home.
void cls() {
#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
	// TODO: This is cheating...
	system("cls");
#else
	rlutil::RLUTIL_PRINT("\033[2J\033[H");
#endif
}

/// Function: locate
/// Sets the cursor position to 1-based x,y.
void locate(int x, int y) {
#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
	COORD coord;
	coord.X = (SHORT)x-1;
	coord.Y = (SHORT)y-1; // Windows uses 0-based coordinates
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
#else // _WIN32 || USE_ANSI
	std::ostringstream oss;
	oss << "\033[" << y << ";" << x << "H";
	rlutil::RLUTIL_PRINT(oss.str());
#endif // _WIN32 || USE_ANSI
}

/// Function: hidecursor
/// Hides the cursor.
void hidecursor() {
#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
	HANDLE hConsoleOutput;
	CONSOLE_CURSOR_INFO structCursorInfo;
	hConsoleOutput = GetStdHandle( STD_OUTPUT_HANDLE );
	GetConsoleCursorInfo( hConsoleOutput, &structCursorInfo ); // Get current cursor size
	structCursorInfo.bVisible = FALSE;
	SetConsoleCursorInfo( hConsoleOutput, &structCursorInfo );
#else // _WIN32 || USE_ANSI
	rlutil::RLUTIL_PRINT("\033[?25l");
#endif // _WIN32 || USE_ANSI
}

/// Function: showcursor
/// Shows the cursor.
void showcursor() {
#if defined(_WIN32) && !defined(RLUTIL_USE_ANSI)
	HANDLE hConsoleOutput;
	CONSOLE_CURSOR_INFO structCursorInfo;
	hConsoleOutput = GetStdHandle( STD_OUTPUT_HANDLE );
	GetConsoleCursorInfo( hConsoleOutput, &structCursorInfo ); // Get current cursor size
	structCursorInfo.bVisible = TRUE;
	SetConsoleCursorInfo( hConsoleOutput, &structCursorInfo );
#else // _WIN32 || USE_ANSI
	rlutil::RLUTIL_PRINT("\033[?25h");
#endif // _WIN32 || USE_ANSI
}

/// Function: msleep
/// Waits given number of milliseconds before continuing.
void msleep(unsigned int ms) {
#ifdef _WIN32
	Sleep(ms);
#else
	// usleep argument must be under 1 000 000
	if (ms > 1000) sleep(ms/1000000);
	usleep((ms % 1000000) * 1000);
#endif
}

/// Function: trows
/// Get the number of rows in the terminal window or -1 on error.
int trows() {
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
		return -1;
	else
		return csbi.srWindow.Bottom - csbi.srWindow.Top + 1; // Window height
		// return csbi.dwSize.Y; // Buffer height
#else
#ifdef TIOCGSIZE
	struct ttysize ts;
	ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
	return ts.ts_lines;
#elif defined(TIOCGWINSZ)
	struct winsize ts;
	ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
	return ts.ws_row;
#else // TIOCGSIZE
	return -1;
#endif // TIOCGSIZE
#endif // _WIN32
}

/// Function: tcols
/// Get the number of columns in the terminal window or -1 on error.
int tcols() {
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if(!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
		return -1;
	} else {
		return csbi.srWindow.Right - csbi.srWindow.Left + 1; // Window width
	}
#else
#ifdef TIOCGSIZE
	struct ttysize ts;
	ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
	return ts.ts_cols;
#elif defined(TIOCGWINSZ)
	struct winsize ts;
	ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
	return ts.ws_col;
#else // TIOCGSIZE
	return -1;
#endif // TIOCGSIZE
#endif // _WIN32
}	

/// Function: anykey
/// Waits until a key is pressed.
void anykey() {
	getch();
}

} // namespace rlutil