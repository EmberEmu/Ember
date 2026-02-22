/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/LogSeverity.h>
#include <ports/pcp/Client.h>
#include <ports/pcp/Daemon.h>
#include <ports/upnp/SSDP.h>
#include <ports/upnp/IGDevice.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <atomic>
#include <functional>
#include <semaphore>
#include <memory>
#include <string>
#include <string_view>
#include <cassert>
#include <cstdint>

namespace ember::ports {

/*
 * Helper class that provides a basic API for forwarding a port for
 * any services that may need to be available over the Internet.
 *
 * This is a best-effort service in that it makes no guarantee that
 * it'll work reliably given the hardware variety and implementation
 * quality of the various protocols.
 */
class Forward final {
	static constexpr auto timeout_interval = 10s;

public:
	using LogCallback = std::function<void(Severity, const std::string_view)>;

	enum class Method {
		upnp,
		pmp_pcp,
		auto_determine
	};

	boost::asio::io_context& ctx_;
	std::uint16_t port_;
	std::atomic_bool mapping_active_;
	std::unique_ptr<ports::Client> client_;
	std::unique_ptr<ports::Daemon> daemon_;
	std::unique_ptr<ports::upnp::SSDP> ssdp_;
	std::shared_ptr<ports::upnp::IGDevice> upnp_device_;
	boost::asio::steady_timer timer_;
	std::binary_semaphore shutdown_wait_;
	LogCallback log_cb_;

	template<typename ... Args>
	void log(Severity severity, const std::format_string<Args...> fmt, Args&& ...args) const;

	std::string_view error_string(const Error& error) const;
	void begin_forwarding(Method Method, const std::string& iface, const std::string& gateway, std::uint16_t port);
	void try_pmp(const std::string& iface, const std::string& gateway, std::uint16_t port);
	void start_upnp(const std::string& iface, std::uint16_t port);
	void start_pmp(const std::string& iface, const std::string& gateway, std::uint16_t port);
	void start_auto(const std::string& iface, const std::string& gateway, std::uint16_t port);

	void unmap_upnp();
	void unmap_pmp();

public:
	Forward(boost::asio::io_context& ctx, Method method, const std::string& iface,
			const std::string& gateway, std::uint16_t port, LogCallback cb = {});

	~Forward();
	void unmap();
};

} // ports, ember