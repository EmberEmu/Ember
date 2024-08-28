/*
The MIT License (MIT)

Copyright (c) 2015 Tobias Widlund

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

#pragma once

#include <logger/Logger.h>
#include <algorithm>
#include <ostream>
#include <source_location>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

namespace smart_enum
{

int char_to_int(char c, int base, bool& err);
unsigned long long sv_to_ull(std::string_view string, bool& err, int base = 0);
std::string_view trimWhitespace(std::string_view str);
std::string_view extractEntry(std::string_view valuesString);
void print_and_bail(std::source_location location);
unsigned long long decimal_convert(std::string_view rhs, bool& conv_err);

static std::string_view unknown_enum_str { "UNKNOWN_ENUM_VALUE" };

template<typename SizeType>
std::unordered_map<SizeType, std::string_view>
makeEnumNameMap(std::source_location location, std::string_view enumValuesString)
{
    std::unordered_map<SizeType, std::string_view> nameMap;
    SizeType currentEnumValue = 0;

    while(!enumValuesString.empty())
    {
        std::string_view currentEnumEntry = extractEntry(enumValuesString);
		size_t nextCommaPos = enumValuesString.find(',');

		if(nextCommaPos != std::string_view::npos) {
			enumValuesString = enumValuesString.substr(nextCommaPos + 1);
		} else {
			enumValuesString = std::string_view{};
		}

        size_t equalSignPos = currentEnumEntry.find('=');
		bool conv_err = false;

        if(equalSignPos != std::string_view::npos)
        {
			std::string_view rightHandSide = trimWhitespace(currentEnumEntry.substr(equalSignPos + 1));
                
            // Handle using util::mcc_char - this makes me sad too
            if (auto first = rightHandSide.find_first_of('"'), last = rightHandSide.find_last_of('"');
                first != std::string_view::npos && first != last) {
                rightHandSide = rightHandSide.substr(first, (last - first) + 1);
            }

			if((rightHandSide[0] == '\'' && rightHandSide[rightHandSide.size() - 1] == '\'')
            || (rightHandSide[0] == '"' && rightHandSide[rightHandSide.size() - 1] == '"')) {
				currentEnumValue = static_cast<SizeType>(decimal_convert(rightHandSide, conv_err));
			} else {
				currentEnumValue = static_cast<SizeType>(sv_to_ull(rightHandSide, conv_err));
			}

			if(conv_err) {
				print_and_bail(location);
			}

            currentEnumEntry = currentEnumEntry.substr(0, equalSignPos-1);
        }
    
        currentEnumEntry = trimWhitespace(currentEnumEntry);
        nameMap[currentEnumValue] = currentEnumEntry;
        ++currentEnumValue;
    }

    return nameMap;
}

}

#define smart_enum(Type, Underlying, ...) enum Type : Underlying { __VA_ARGS__}; \
    static std::unordered_map<Underlying, std::string_view> Type##_enum_names\
		= smart_enum::makeEnumNameMap<Underlying>(std::source_location::current(), #__VA_ARGS__);\
    \
    inline std::string_view Type##_to_string(Type value) try \
    { \
        return Type##_enum_names.at((Underlying)value);\
    } catch(std::out_of_range&) { \
		return smart_enum::unknown_enum_str; \
	} \
    \

#define smart_enum_class(Type, Underlying, ...) enum class Type : Underlying { __VA_ARGS__}; \
    static std::unordered_map<Underlying, std::string_view> Type##_enum_names\
		= smart_enum::makeEnumNameMap<Underlying>(std::source_location::current(), #__VA_ARGS__);\
    \
    inline std::string_view to_string(Type value) try \
    { \
        return Type##_enum_names.at((Underlying)value);\
    } catch(std::out_of_range&) { \
		return smart_enum::unknown_enum_str; \
	} \
    \
    inline std::ostream& operator<<(std::ostream& outStream, Type value)\
    {\
        outStream << to_string(value);\
        return outStream;\
    } \
    inline log::Logger& operator <<(log::Logger& outStream, Type value) \
    {\
        outStream << to_string(value);\
        return outStream;\
    }\

#define CREATE_FORMATTER(name)\
	template<>\
	struct std::formatter<name, char> {\
		template<typename ParseContext>\
		constexpr ParseContext::iterator parse(ParseContext& ctx) {\
			return ctx.begin();\
		}\
	\
		template<typename FmtContext>\
		FmtContext::iterator format(name value, FmtContext& ctx) const {\
			const auto& str = to_string(value);\
			return std::ranges::copy(str.begin(), str.end(), ctx.out()).out;\
		}\
	};