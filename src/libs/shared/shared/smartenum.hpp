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
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace smart_enum
{
    inline int determine_base(std::string& rightHandSide)
    {
        std::transform(rightHandSide.begin(), rightHandSide.end(), rightHandSide.begin(), ::tolower);

        int base = 0;

        if(rightHandSide.compare(0, 2, "0b") == 0)
        { 
            rightHandSide = rightHandSide.substr(2, std::string::npos);
            base = 2;
        }

        return base;
    }

    inline std::string trimWhitespace(std::string str)
    {
        // trim trailing whitespace
        size_t endPos = str.find_last_not_of(" \t");
        if(std::string::npos != endPos)
        {
            str = str.substr( 0, endPos + 1);
        }
    
        // trim leading spaces
        size_t startPos = str.find_first_not_of(" \t");
        if(std::string::npos != startPos)
        {
            str = str.substr(startPos);
        }
    
        return str;
    }
    
    inline std::string extractEntry(std::string& valuesString)
    {
        std::string result;
        size_t nextCommaPos = valuesString.find(',');
    
        if(nextCommaPos != std::string::npos)
        {
            std::string segment = valuesString.substr(0, nextCommaPos);
            valuesString.erase(0, nextCommaPos + 1);
            result = trimWhitespace(segment);
        }
        else
        {
            result = trimWhitespace(valuesString);
            valuesString.clear();
        };
        return result;
    };
    
    template<typename SizeType, typename Type>
    inline std::unordered_map<SizeType, std::string> makeEnumNameMap(std::vector<Type>& list, std::string enumValuesString)
    {
        std::unordered_map<SizeType, std::string> nameMap;
        SizeType currentEnumValue = 0;

        while(!enumValuesString.empty())
        {
            std::string currentEnumEntry = extractEntry(enumValuesString);
            size_t equalSignPos = currentEnumEntry.find('=');

            if(equalSignPos != std::string::npos)
            {
                std::string rightHandSide = trimWhitespace(currentEnumEntry.substr(equalSignPos + 1));
                
                // Handle using util::mcc_char - this makes me sad too
                if (auto first = rightHandSide.find_first_of('"'), last = rightHandSide.find_last_of('"');
                    first != std::string::npos && first != last) {
                    rightHandSide = rightHandSide.substr(first, (last - first) + 1);
                }

				if((rightHandSide[0] == '\'' && rightHandSide[rightHandSide.size() - 1] == '\'')
                || (rightHandSide[0] == '"' && rightHandSide[rightHandSide.size() - 1] == '"')) {
					std::stringstream decimalConvert;
					decimalConvert << "0x" << std::hex << std::setw(2) << std::setfill('0');

					for(std::size_t i = 1; i < rightHandSide.size() - 1; ++i) {
						decimalConvert << static_cast<unsigned>(rightHandSide[i]);
					}

					rightHandSide = decimalConvert.str();
				}
                currentEnumValue = static_cast<SizeType>(std::stoull(rightHandSide, 0, determine_base(rightHandSide)));
                currentEnumEntry.erase(equalSignPos);
            }
    
            currentEnumEntry = trimWhitespace(currentEnumEntry);
            list.emplace_back(static_cast<Type>(currentEnumValue));
            nameMap[currentEnumValue] = currentEnumEntry;
    
            currentEnumValue++;
        }
        
        return nameMap;
	}
}

#define smart_enum(Type, Underlying, ...) enum Type : Underlying { __VA_ARGS__}; \
    static std::vector<Type> Type##_list;\
    static std::unordered_map<Underlying, std::string> Type##_enum_names = smart_enum::makeEnumNameMap<Underlying, Type>(Type##_list, #__VA_ARGS__);\
    \
    inline const std::string& Type##_to_string(Type value) try \
    { \
        return Type##_enum_names.at((Underlying)value);\
    } catch(std::out_of_range&) { \
		return "UNKNOWN_ENUM_VALUE"; \
	} \
    \

#define smart_enum_class(Type, Underlying, ...) enum class Type : Underlying { __VA_ARGS__}; \
    static std::vector<Type> Type##_list;\
    static std::unordered_map<Underlying, std::string> Type##_enum_names = smart_enum::makeEnumNameMap<Underlying, Type>(Type##_list, #__VA_ARGS__);\
    \
    inline const std::string& to_string(Type value) try \
    { \
        return Type##_enum_names.at((Underlying)value);\
    } catch(std::out_of_range&) { \
		return "UNKNOWN_ENUM_VALUE"; \
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
		template<class ParseContext>\
		constexpr ParseContext::iterator parse(ParseContext& ctx) {\
			return ctx.begin();\
		}\
	\
		template<class FmtContext>\
		FmtContext::iterator format(name value, FmtContext& ctx) const {\
			std::ostringstream out;\
			out << value;\
			return std::ranges::copy(std::move(out).str(), ctx.out()).out;\
		}\
	};
