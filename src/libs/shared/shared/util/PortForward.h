/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Logging.h>
#include <ports/pcp/Client.h>
#include <ports/pcp/Daemon.h>
#include <ports/upnp/SSDP.h>
#include <ports/upnp/IGDevice.h>
#include <boost/asio/io_context.hpp>
#include <memory>
#include <string>
#include <cassert>
#include <cstdint>

namespace ember::util {

class PortForward {
public:
	enum class Mode {
		UPNP, PMP_PCP
	};

private:
	Mode mode_;
	log::Logger* logger_;
	std::unique_ptr<ports::Client> client_;
	std::unique_ptr<ports::Daemon> daemon_;
	std::unique_ptr<ports::upnp::SSDP> ssdp_;

public:
	PortForward(log::Logger* logger, boost::asio::io_context& ctx, Mode mode,
				const std::string& iface, const std::string& gateway, std::uint16_t port)
		: logger_(logger) {
		switch(mode) {
			case Mode::UPNP:
				start_upnp(ctx, iface, port);
				break;
			case Mode::PMP_PCP:
				start_pmp(ctx, iface, gateway, port);
				break;
			default:
				assert("Bad port forwarding method specified");
		}
	}

	void start_upnp(boost::asio::io_context& ctx, const std::string& iface, std::uint16_t port) {
		ssdp_ = std::make_unique<ports::upnp::SSDP>(iface, ctx);

		ssdp_->locate_gateways([&](ports::upnp::LocateResult result) {
			if(!result) {
				LOG_ERROR_FMT(logger_, "Port forwarding failed with error code {}",
				              result.error().value());
				return true;
			}

			ports::upnp::Mapping map {
				.external = port,
				.internal = port,
				.ttl = 0,
				.protocol = ports::Protocol::TCP
			};

			auto callback = [&, map](ports::upnp::ErrorCode ec) {
				if(!ec) {
					LOG_INFO_FMT(logger_, "Port {} forwarded using UPnP", map.external);
				} else {
					LOG_ERROR_FMT(logger_, "Port {} forwarding failed using UPnP, error {}",
						map.external, ec.value());
				}
			};
		
			result->device->add_port_mapping(map, callback);
			return false;
		});
	}

	void start_pmp(boost::asio::io_context& ctx, const std::string& iface,
	               const std::string& gateway, std::uint16_t port) {
		const ports::MapRequest request {
			.protocol      = ports::Protocol::TCP,
			.internal_port = port,
			.external_port = port,
			.lifetime      = 7200u
		};

		client_ = std::make_unique<ports::Client>(iface, gateway, ctx);
		daemon_ = std::make_unique<ports::Daemon>(*client_, ctx);

		daemon_->add_mapping(request, false, [&](const ports::Result& result) {
			if(result) {
				LOG_INFO_FMT(logger_, "Port {} -> {} forwarded for {} seconds\n",
						     result->external_port,
						     result->internal_port,
						     result->lifetime);
			} else {
				LOG_ERROR(logger_) << "Port forwarding failed" << LOG_ASYNC;
			}
		});
	}
};

} // util, ember