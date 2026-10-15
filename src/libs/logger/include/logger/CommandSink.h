/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Sink.h>
#include <commands/Suggestions.h>
#include <functional>
#include <string>
#include <string_view>

namespace ember::log {

class CommandSink : public Sink {
public:
	static constexpr std::string_view sink_name = "CommandSink";

	using CommandHandler = std::function<void(const std::string_view)>;
	using Autocomplete = std::function<commands::Suggestions(const std::string&)>;
	using Suggestion = std::function<std::string(const std::string_view)>;

	CommandSink(Severity severity, Filter filter);
	~CommandSink() {}

	void write(Severity severity, Filter type, std::string_view record, bool flush) override = 0;
	void batch_write(const std::span<std::pair<RecordDetail, std::string>>& records) override = 0;

	virtual void colourise(bool colourise) = 0;
	virtual void prefix(std::string prefix) = 0;

	virtual void register_handler(CommandHandler handler) = 0;
	virtual void register_autocomplete(Autocomplete handler) = 0;
	virtual void register_suggestion(Suggestion handler) = 0;

	virtual bool invoke(const std::string_view command) = 0;
	virtual void clear_console() = 0;
	virtual bool unique() override = 0;
};

} // log, ember