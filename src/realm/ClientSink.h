/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Events.h"
#include <logger/Sink.h>
#include <shared/ClientIdent.h>
#include <string>
#include <string_view>
#include <cstdint>

namespace ember::realm {

class EventDispatcher;

/*
 * This sink enables forwarding log messages to interested game clients.
 * It does this by dispatching events to the provided ClientIdent, which allows for
 * safely addressing a specific client across threads.
 * 
 * Each client registers its own sink rather than sharing a single sink just for
 * keeping the implementation as simple as possible and reducing overhead in the
 * common case of there being no interested clients.
 * 
 * This is purely intended for development and debugging.
 */
class ClientSink final : public log::Sink {
	static constexpr std::string_view sink_type { "ClientSink" };
	static constexpr auto reserve_size = 128;

	enum class Colour : std::uint32_t {
		alpha_grey = 0x80808080,
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
	const LogRedirect::Type type_;
	EventDispatcher& dispatcher_;

	Colour severity_rgb(log::Severity severity) const;
	std::string format(std::string_view prefix, std::string_view message, log::Severity severity) const;
	void dispatch(std::string message);
	bool filtered(log::Severity severity, log::Filter type) const;

public:
	ClientSink(std::string name, EventDispatcher& dispatcher, ClientIdent client,
	           log::Severity severity, LogRedirect::Type type,
	           log::Filter filter);

	void write(log::Severity severity, log::Filter type, std::string_view prefix, std::string_view record, bool flush) override;
	void batch_write(const std::span<std::tuple<log::RecordDetail, std::string_view, std::string>>& records) override;

	static constexpr std::string_view type() { return sink_type; }
};

} // realm, ember