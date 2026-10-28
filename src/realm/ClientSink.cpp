/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ClientSink.h"
#include "EventDispatcher.h"
#include <logger/Utility.h>
#include <utility>

namespace ember::realm {

ClientSink::ClientSink(std::string name, EventDispatcher& dispatcher, ClientIdent client,
                       log::Severity severity, LogRedirect::Type type, log::Filter filter)
	: log::Sink(std::move(name), std::string(sink_type), severity, filter)
	, dispatcher_(dispatcher)
	, severity_(severity)
	, filter_(filter)
	, client_(client)
	, type_(type) {}

void ClientSink::dispatch(std::string message) {
	LogRedirect event(std::move(message), type_);
	dispatcher_.post(client_, std::move(event));
}

inline bool ClientSink::filter(log::Severity severity, log::Filter type) const {
	return severity < severity_ || filter_ & type;
}

void ClientSink::write(log::Severity severity, log::Filter type, std::string_view prefix, std::string_view record, bool) {
	if(filter(severity, type)) {
		return;
	}

	// log entries always end with a newline to allow for fast bulk processing
	// without manipulating the data but the game client handles its own newlines,
	// so we need to trim it from the end here or we'd get double newlines
	if(record.ends_with('\n')) {
		record.remove_suffix(1);
	}

	auto message = format(prefix, record, severity);
	dispatch(std::move(message));
}

void ClientSink::batch_write(const std::span<std::tuple<log::RecordDetail, std::string_view, std::string>>& records) {
	std::string messages;
	messages.reserve(reserve_size * records.size());

	for(auto& [detail, prefix, data] : records) {
		if(filter(detail.severity, detail.type)) {
			return;
		}

		std::string_view record(data);
		messages.append(format(prefix, record, detail.severity));
	}

	// see comment in 'write'
	if(messages.ends_with('\n')) {
		messages.pop_back();
	}

	dispatch(std::move(messages));
}

auto ClientSink::severity_rgb(log::Severity severity) const -> Colour {
	using enum log::Severity;

	switch(severity) {
		case trace:
			return Colour::alpha_grey;
		case debug:
			return Colour::light_grey;
		case info:
			return Colour::white;
		case warn:
			return Colour::light_red;
		case error:
			return Colour::red;
		case fatal:
			return Colour::red;
		case console:
			return Colour::green;
		case console_error:
			return Colour::red;
		default:
			return Colour::white;
	}
}

std::string ClientSink::format(const std::string_view prefix, const std::string_view message, log::Severity severity) const {
	const auto colour = std::to_underlying(severity_rgb(severity));
	const auto sev_str = log::severity_string(severity);

	if(prefix.empty()) {
		return std::format("{} |c{:x}{}|r", sev_str, colour, message);
	} else {
		return std::format("{} {} |c{:x}{}|r", sev_str, prefix, colour, message);
	}
}

} // realm, ember