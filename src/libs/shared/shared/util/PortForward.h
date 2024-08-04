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
#include <semaphore>
#include <string>
#include <cassert>
#include <cstdint>

namespace ember::util {

/*
 * Helper class that provides a basic API for forwarding a port for
 * any services that may need to be available over the Internet.
 *
 * This is a best-effort service in that it makes no guarantee that
 * it'll work reliably given the hardware variety and implementation
 * quality of the various protocols out there. If it fails, the user 
 * should configure the ports manually.
 */
class PortForward {
public:
	enum class Mode {
		UPNP, PMP_PCP
	};

private:
	log::Logger* logger_;
	std::uint16_t port_;
	bool mapping_active_;
	std::unique_ptr<ports::Client> client_;
	std::unique_ptr<ports::Daemon> daemon_;
	std::unique_ptr<ports::upnp::SSDP> ssdp_;
	std::shared_ptr<ports::upnp::IGDevice> upnp_device_;
	std::binary_semaphore sem_;

	void start_upnp(boost::asio::io_context& ctx, const std::string& iface, std::uint16_t port) {
		ssdp_ = std::make_unique<ports::upnp::SSDP>(iface, ctx);

		ssdp_->locate_gateways([&, port](ports::upnp::LocateResult result) {
			if(!result) {
				LOG_ERROR_FMT(logger_, "UPnP gateway search failed with error code {}",
				              result.error().value());
				return true;
			}

			ports::upnp::Mapping map {
				.external = port,
				.internal = port,
				.ttl = 0,
				.protocol = ports::Protocol::TCP
			};

			auto callback = [&](ports::upnp::ErrorCode ec) {
				if(!ec) {
					LOG_INFO_FMT(logger_, "Port {} successfully forwarded (UPnP)", port);
					mapping_active_ = true;
				} else {
					LOG_ERROR_FMT(logger_, "Port forwarding failed (UPnP), error {}", ec.value());
				}
			};
		
			result->device->add_port_mapping(map, callback);
			upnp_device_ = std::move(result->device);
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
				LOG_INFO_FMT(logger_, "Port {} -> {} forwarded for {} seconds (NATPMP/PCP)",
						     result->external_port,
						     result->internal_port,
						     result->lifetime);
				mapping_active_ = true;
			} else {
				LOG_ERROR_FMT(logger_, "Port forwarding failed (NATPMP/PCP), error {}",
				              result.error().code);
				mapping_active_ = false; // mapping can succeed and later fail when refreshed
			}
		});
	}

	void unmap_upnp() {
		assert(upnp_device_);

		const ports::upnp::Mapping map {
			.external = port_,
			.internal = port_,
			.ttl = 0,
			.protocol = ports::Protocol::TCP
		};

		auto callback = [&](ports::upnp::ErrorCode ec) {
			if(!ec) {
				LOG_INFO_FMT(logger_, "Successfully unmapped forwarded port {}", port_);
			} else {
				LOG_ERROR_FMT(logger_, "Failed to unmap forwarded port {}, error ",
				              ec.value());
			}

			sem_.release();
		};
		
		upnp_device_->delete_port_mapping(map, callback);
		sem_.acquire();
	}

	void unmap_pmp() {
		assert(daemon_);

		daemon_->delete_mapping(port_, ports::Protocol::TCP, [&](const ports::Result& result) {
			if(result) {
				LOG_INFO_FMT(logger_, "Successfully unmapped forwarded port", port_);
			} else {
				LOG_ERROR_FMT(logger_, "Failed to unmap forwarded port, error {}",
				              result.error().code);
			}

			sem_.release();
		});

		sem_.acquire();
	}

public:
	PortForward(log::Logger* logger, boost::asio::io_context& ctx, Mode mode,
				const std::string& iface, const std::string& gateway, std::uint16_t port)
		: logger_(logger), port_(port), mapping_active_(false), sem_(0) {
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

	~PortForward() {
		unmap();
	}

	void unmap() {
		if(!mapping_active_) {
			return;
		}

		LOG_INFO_FMT(logger_, "Attempting to remove port mapping for {}", port_);

		if(daemon_) {
			unmap_pmp();
		} else if(upnp_device_) {
			unmap_upnp();
		}

		// if it failed, too bad, nothing we can do
		mapping_active_ = false;
	}
};

} // util, ember