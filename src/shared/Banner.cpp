#pragma once

#include "Banner.h"
#include "Version.h"
#include <iostream>

namespace ember {

void print_banner(const std::string& display_name) {
	std::cout << "\n"
		R"(                                      d8b                       )" << "\n"
		R"(                                      ?88                       )" << "\n"
		R"(                                       88b                      )" << "\n"
		R"(                 d8888b  88bd8b,d88b   888888b  d8888b  88bd88b )" << "\n"
		R"(                d8b_,dP  88P'`?8P'?8b  88P `?8bd8b_,dP  88P'  ` )" << "\n"
		R"(                88b     d88  d88  88P d88,  d8888b     d88      )" << "\n"
		R"(                `?888P'd88' d88'  88bd88'`?88P'`?888P'd88'      )" << "\n\n";
		
	std::cout << display_name << ", " << version::VERSION << " (" << version::GIT_HASH << ")" << std::endl;
}

}