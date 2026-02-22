/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ports/Forward.h>
#include <boost/asio/post.hpp>
#include <format>

namespace ember::ports {

Forward::Forward(boost::asio::io_context& ctx, Method method, const std::string& iface,
                 const std::string& gateway, std::uint16_t port, LogCallback cb)
	: ctx_(ctx),
	  port_(port),
	  mapping_active_(false),
	  timer_(ctx),
	  shutdown_wait_(0),
	  log_cb_(cb) {
	begin_forwarding(method, iface, gateway, port);
}

Forward::~Forward() {
	unmap();
}

template<typename ...Args>
void Forward::log(Severity severity, const std::format_string<Args...> fmt, Args&& ...args) const {
	if(!log_cb_) {
		return;
	}

	const auto formatted = std::format(fmt, std::forward<Args>(args)...);
	log_cb_(severity, formatted);
}

void Forward::start_upnp(const std::string& iface, std::uint16_t port) {
	ssdp_ = std::make_unique<ports::upnp::SSDP>(iface, ctx_);

	ssdp_->locate_gateways([&, port](ports::upnp::LocateResult result) {
		if(!result) {
			log(
				Severity::error,
				"UPnP gateway search failed with error code {}", result.error().value()
			);

			return true;
		}

		const ports::upnp::Mapping map {
			.external = port,
			.internal = port,
			.ttl = 0,
			.protocol = ports::Protocol::tcp
		};

		result->device->add_port_mapping(map, [&](ports::upnp::ErrorCode ec) {
			if(!ec) {
				log(Severity::info, "[UPnP] Port {} successfully forwarded", port);
				mapping_active_ = true;
			} else {
				log(Severity::error, "[UPnP] Port forwarding failed, error {}", ec.value());
			}

		});

		upnp_device_ = std::move(result->device);
		return false;
	});
}

void Forward::start_pmp(const std::string& iface, const std::string& gateway, std::uint16_t port) {
	const ports::MapRequest request {
		.protocol      = ports::Protocol::tcp,
		.internal_port = port,
		.external_port = port,
		.lifetime      = 7200u
	};

	client_ = std::make_unique<ports::Client>(iface, gateway, ctx_);

	daemon_ = std::make_unique<ports::Daemon>(*client_, ctx_, [&](auto event, const auto& result) {
		const auto v6 = boost::asio::ip::address_v6(result.external_ip);
	
		switch(event) {
			case ports::Daemon::Event::added_mapping:
				log(
					Severity::debug,
					"[NATPMP/PCP][Daemon] {}:{} -> {} forwarded for {} seconds",
					v6.to_string(), result.external_port, result.internal_port, result.lifetime
				);
				break;
			case ports::Daemon::Event::failed_mapping:
				log(Severity::debug,
					"[NATPMP / PCP][Daemon] Port forwarding attempt failed, will retry...",
					v6.to_string(), result.external_port, result.internal_port, result.lifetime
				);
				break;
			case ports::Daemon::Event::renewed_mapping:
				log(Severity::debug,
					"[NATPMP/PCP] [Daemon] Port {} forwarding renewed for {} seconds",
					result.external_port, result.lifetime
				);
				break;
			case ports::Daemon::Event::failed_to_renew:
				log(
					Severity::warn,
					"[NATPMP/PCP] [Daemon] Port forwarding renewal failure, will retry..."
				);
				break;
			case ports::Daemon::Event::mapping_expired:
				log(Severity::warn,
					"[NATPMP/PCP] [Daemon] Port forwarding expired, service may be unavailable"
					"to outside hosts, remapping will be attempted"
				);
				break;
		}
	});

	daemon_->add_mapping(request, false, [&, request](const ports::Result& result) {
		if(result) {
			const auto v6 = boost::asio::ip::address_v6(result->external_ip);

			log(Severity::info,
				"[NATPMP/PCP] {}:{} -> {} forwarded for {} seconds (requested {} seconds)",
				v6.to_string(),
			    result->external_port,
			    result->internal_port,
			    result->lifetime,
				request.lifetime
			);
			mapping_active_ = true;
		} else {
			log(Severity::error,
				R"([NATPMP/PCP] Port forwarding failed, error "{}")", error_string(result.error())
			);
			mapping_active_ = false; // mapping can succeed and later fail when refreshed
		}
	});
}

std::string_view Forward::error_string(const Error& error) const {
	if(error.code == ErrorCode::natpmp_code) {
		return to_string(error.natpmp_code);
	} else if(error.code == ErrorCode::pcp_code) {
		return to_string(error.pcp_code);
	} else {
		return to_string(error.code);
	}
}

void Forward::unmap_upnp() {
	assert(upnp_device_);

	const ports::upnp::Mapping map {
		.external = port_,
		.internal = port_,
		.ttl = 0,
		.protocol = ports::Protocol::tcp
	};

	upnp_device_->delete_port_mapping(map, [&](ports::upnp::ErrorCode ec) {
		if(!ec) {
			log(Severity::info,
				"[UPnP] Successfully unmapped forwarded port {}", port_
			);
		} else {
			log(Severity::info,
				"[UPnP] Failed to unmap forwarded port {}, error ", ec.value()
			);
		}

		shutdown_wait_.release();
	});
}

void Forward::unmap_pmp() {
	assert(daemon_);

	daemon_->delete_mapping(port_, ports::Protocol::tcp, [&](const ports::Result& result) {
		if(result) {
			log(Severity::info,
				"[NATPMP/PCP] Successfully unmapped forwarded port", port_
			);
		} else {
			log(Severity::error,
				"[NATPMP/PCP] Failed to unmap forwarded port, error {}", result.error().code
			);
		}

		shutdown_wait_.release();
	});
}

void Forward::try_pmp(const std::string& iface, const std::string& gateway, std::uint16_t port) {
	start_pmp(iface, gateway, port);

	timer_.expires_after(timeout_interval);
	timer_.async_wait([&, iface, gateway, port](auto ec) {
		if(ec) {
			return;
		}

		if(mapping_active_) {
			return;
		}
		
		log(Severity::warn,
			"Automatic port forwarding not permitted by NAT gateway"
		    " or gateway address is incorrectly configured"
		);
	});
}

void Forward::start_auto(const std::string& iface, const std::string& gateway, std::uint16_t port) {
	log(Severity::info, "Attempting port forwarding with UPnP...");

	start_upnp(iface, port);

	timer_.expires_after(timeout_interval);
	timer_.async_wait([&, iface, gateway, port](auto ec) {
		if(ec) {
			return;
		}

		if(mapping_active_) {
			return;
		}
		
		log(Severity::info, "Attempting port forwarding with NAT-PMP & PCP...");
		try_pmp(iface, gateway, port);
	});
}

void Forward::begin_forwarding(Method method, const std::string& iface, const std::string& gateway, std::uint16_t port) {
	switch(method) {
		case Method::upnp:
			start_upnp(iface, port);
			break;
		case Method::pmp_pcp:
			start_pmp(iface, gateway, port);
			break;
		case Method::auto_determine:
			start_auto(iface, gateway, port);
			break;
		default:
			assert("Bad port forwarding method specified");
	}
}

void Forward::unmap() {
	if(!mapping_active_) {
		return;
	}

	if(daemon_) {
		unmap_pmp();
	} else if(upnp_device_) {
		unmap_upnp();
	}

	// if it failed, too bad, nothing we can do
	mapping_active_ = false;
	shutdown_wait_.acquire();
}

} // ports, ember