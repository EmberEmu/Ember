/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Events.h"
#include "EventDispatcher.h"
#include <logger/Sink.h>
#include <logger/Severity.h>
#include <shared/ClientIdent.h>
#include <string>
#include <cstdint>

namespace ember::realm {

class ClientSink final : public log::Sink {

	enum class Colour : std::uint32_t {
		grey_alpha = 0x80808080,
		light_grey = 0xffd4d4d4,
		white      = 0xffffffff,
		light_red  = 0xffff5757,
		blue       = 0xff006eff,
		green      = 0xff48ff00,
		purple     = 0xff9000ff,
		red        = 0xffff002b,
		yellow     = 0xffffff00,
	};

	const ClientIdent client_;
	const log::Severity severity_;
	const log::Filter filter_;
	const LogRedirect::Type type_;
	EventDispatcher& dispatcher_;

	Colour severity_rgb(log::Severity severity) const;
	std::string format(std::string_view input, log::Severity severity) const;

public:
	static constexpr std::string_view sink_name = "ClientSink";

	ClientSink(EventDispatcher& dispatcher, log::Severity severity, log::Filter filter, ClientIdent client, LogRedirect::Type type);

	void write(log::Severity severity, log::Filter type, std::span<const char> record, bool flush) override;
	void batch_write(const std::span<std::pair<log::RecordDetail, std::vector<char>>>& records) override;
};

} // realm, ember