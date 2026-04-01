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

void ClientSink::write(log::Severity severity, log::Filter type, std::span<const char> record, bool) {
	if(severity < severity_  || filter() & type) {
		return;
	}

	auto record_sv = std::string_view(record);
	record_sv.remove_suffix(1);
	auto message = format(record_sv, severity);
	auto event = std::make_unique<LogRedirect>(std::move(message), type_);
	dispatcher_.post_event(client_, std::move(event));
}

void ClientSink::batch_write(const std::span<std::pair<log::RecordDetail, std::vector<char>>>& records) {
	for(auto& [detail, data] : records) {
		write(detail.severity, detail.type, data, false);
	}
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
	}

	return Colour::white;
}

std::string ClientSink::format(const std::string_view input, log::Severity severity) const {
	const auto colour = std::to_underlying(severity_rgb(severity));
	const auto sev_str = log::severity_string(severity);
	return std::format("{} |c{:x}{}|r", sev_str, colour, input);
}

} // realm, ember