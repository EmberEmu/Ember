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
#include <memory>
#include <utility>

namespace ember::realm {

ClientSink::ClientSink(EventDispatcher& dispatcher, ClientIdent client, log::Severity severity,
                       LogRedirect::Type type, log::Filter filter)
	: log::Sink(severity, filter, sink_name)
	, dispatcher_(dispatcher)
	, severity_(severity)
	, filter_(filter)
	, client_(client)
	, type_(type) {}

void ClientSink::dispatch(std::string message) {
	auto event = std::make_unique<LogRedirect>(std::move(message), type_);
	dispatcher_.post(client_, std::move(event));
}

inline bool ClientSink::filter(log::Severity severity, log::Filter type) const {
	return severity < severity_ || filter_ & type;
}

void ClientSink::write(log::Severity severity, log::Filter type, std::span<const char> record, bool) {
	if(filter(severity, type)) {
		return;
	}

	auto record_sv = std::string_view(record);

	// log entries always end with a newline to allow for fast bulk processing
	// without manipulating the data but the game client handles its own newlines,
	// so we need to trim it from the end here or we'd get double newlines
	if(record_sv.ends_with('\n')) {
		record_sv.remove_suffix(1);
	}

	auto message = format(record_sv, severity);
	dispatch(std::move(message));
}

void ClientSink::batch_write(const std::span<std::pair<log::RecordDetail, std::vector<char>>>& records) {
	std::string messages;
	messages.reserve(reserve_size * records.size());

	for(auto& [detail, data] : records) {
		if(filter(detail.severity, detail.type)) {
			return;
		}

		auto record = std::string_view(data);
		messages.append(format(record, detail.severity));
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

std::string ClientSink::format(const std::string_view input, log::Severity severity) const {
	const auto colour = std::to_underlying(severity_rgb(severity));
	const auto sev_str = log::severity_string(severity);
	return std::format("{} |c{:x}{}|r", sev_str, colour, input);
}

} // realm, ember