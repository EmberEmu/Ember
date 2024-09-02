/*
The MIT License (MIT)

Copyright (c) 2015 Tobias Widlund
Copyright (c) 2024 Ember

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "smartenum.hpp"
#include <format>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace smart_enum {

int char_to_int(char c, int base, bool& err)
{
	err = false;

	if(c >= '0' && c <= '9') {
		const int val = c - '0';

		if(val >= base) {
			err = true;
		}

		return val;
	}

	c = ::tolower(c);

	if(c >= 'a' && c <= 'z') {
		const int val = c - 'a' + 10;

		if(val >= base) {
			err = true;
		}

		return val;
	}

	err = true;
	return 0;
}

unsigned long long sv_to_ull(std::string_view string, bool& err, int base) {
	if(base == 0) {
		if(string.starts_with("0x") || string.starts_with("0X")) {
			base = 16;
			string = string.substr(2);
		} else if(string.starts_with("0b") || string.starts_with("0B")) {
			base = 2;
			string = string.substr(2);
		} else if(string.starts_with("0")) {
			base = 8;
			string = string.substr(1);
		} else {
			base = 10;
		}
	} else if(base == 2 || base == 16) {
		string = string.substr(2);
	} else if(base == 8) {
		string = string.substr(1);
	} 

	auto value = 0ull;

	for(const auto& byte : string) {
		auto digit = char_to_int(byte, base, err);
		value = (value * base) + (digit);
	}

	return value;
}

std::string_view trimWhitespace(std::string_view str)
{
    // trim trailing whitespace
	const size_t endPos = str.find_last_not_of(" \t");
    if(std::string_view::npos != endPos)
    {
        str = str.substr(0, endPos + 1);
    }
    
    // trim leading spaces
    const size_t startPos = str.find_first_not_of(" \t");
    if(std::string_view::npos != startPos)
    {
        str = str.substr(startPos);
    }
    
    return str;
}
    
std::string_view extractEntry(std::string_view valuesString)
{
	const size_t nextCommaPos = valuesString.find(',');
    
    if(nextCommaPos != std::string_view::npos)
    {
        std::string_view segment = valuesString.substr(0, nextCommaPos);
        return trimWhitespace(segment);
    }
    else
    {
		return trimWhitespace(valuesString);
    };
};

void print_and_bail(std::source_location location) {
	auto msg = std::format("enum parse failed! {}:#L{}", location.file_name(),location.line());
	std::cerr << msg;
	std::exit(EXIT_FAILURE);
}

unsigned long long decimal_convert(std::string_view rhs, bool& conv_err) {
	std::stringstream decimalConvert;
	decimalConvert << "0x" << std::hex << std::setw(2) << std::setfill('0');

	for(std::size_t i = 1; i < rhs.size() - 1; ++i) {
		decimalConvert << static_cast<unsigned>(rhs[i]);
	}

	const auto str = decimalConvert.view();
	return sv_to_ull(str, conv_err);
}
    
} // smartenum