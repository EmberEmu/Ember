/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <commands/ParseArgumentBase.h>
#include <commands/Exception.h>
#include <format>
#include <string>
#include <sstream>

namespace ember::commands {

template<typename T>
class ParseArgument : public ParseArgumentBase {
public:
	std::any to_any(const std::string& token) override {
		T value{};
		std::istringstream stream(token);
		
		try {
			stream >> value;
		} catch(const std::exception& e) {
			auto msg = std::format("{}: {}", token, e.what());
			throw parse_error(std::move(msg));
		}

		if(!stream || stream.peek() != std::char_traits<char>::eof()) {
			auto msg = std::format("{}", token);
			throw parse_error(std::move(msg));
		}

		return value;
	}
};

template<>
class ParseArgument<std::string> : public ParseArgumentBase {
public:
	std::any to_any(const std::string& token) override {
		return token;
	}
};

template<>
class ParseArgument<bool> : public ParseArgumentBase {
public:
	std::any to_any(const std::string& token) override {
		if(token == "on" || token == "enable" || token == "1" || token == "true") {
			return true;
		} else if(token == "off" || token == "disable" || token == "0" || token == "false") {
			return false;
		}

		throw parse_error(
			std::format("Unable to convert '{}' to bool argument", token)
		);
	}
};

} // commands, ember