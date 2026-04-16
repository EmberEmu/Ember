/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ports/upnp/SSDP.h>
#include <ports/upnp/HTTPHeaderParser.h>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/post.hpp>
#include <format>
#include <regex>

namespace ember::ports::upnp {

namespace asio = boost::asio;

SSDP::SSDP(const std::string& bind, boost::asio::io_context& ctx)
	: ctx_(ctx),
	  strand_(ctx),
	  transport_(ctx, bind, mcast_ipv4_addr, mcast_dest_port) {
	asio::co_spawn(ctx_, read_broadcasts(), asio::detached);
}

asio::awaitable<void> SSDP::read_broadcasts() {
	while(true) {
		auto result = co_await transport_.receive();

		if(result) {
			auto ec = validate_message(*result);

			if(ec && handler_) {
				handler_(std::unexpected(ec));
			} else if(handler_) {
				auto locate_res = build_locate_result(*result);
				const bool call_again = handler_(std::move(locate_res));

				if(!call_again) {
					handler_ = {};
				}
			}

			continue;
		} else if(result.error() != boost::asio::error::operation_aborted) {
			handler_(std::unexpected(ErrorCode::network_failure));
		}

		break;
	}
}

LocateResult SSDP::build_locate_result(std::span<const std::uint8_t> datagram) {
	std::string_view txt(
		reinterpret_cast<const char*>(datagram.data()), datagram.size()
	);

	HTTPHeader header;
	parse_http_header(txt, header);

	std::string location(header.fields["Location"]);
	std::string service(header.fields["ST"]);

	try {
		auto bind = transport_.local_address();
		auto device = std::make_shared<IGDevice>(
			ctx_, std::move(bind), std::move(service), std::move(location)
		);

		DeviceResult result {
			.header = std::move(header),
			.device = std::move(device),
		};
		
		return result;
	} catch(const std::exception&) {
		return std::unexpected(ErrorCode::http_bad_response);
	}
}

ErrorCode SSDP::validate_message(std::span<const std::uint8_t> datagram) {
	std::string_view txt(
		reinterpret_cast<const char*>(datagram.data()), datagram.size()
	);

	HTTPHeader header;

	if(!parse_http_header(txt, header)) {
		return ErrorCode::http_bad_headers;
	}

	if(header.code != HTTPStatus::ok) {
		return ErrorCode::http_not_ok;
	}

	const auto& location = header.fields["Location"];
	const auto& service = header.fields["ST"];

	if(location.empty() || service.empty()) {
		return ErrorCode::http_header_field_awol;
	}

	return ErrorCode::success;
}

std::vector<std::uint8_t> SSDP::build_ssdp_request(const std::string_view type,
                                                   std::string_view subtype,
                                                   const int version) {
	constexpr std::string_view request {
		R"(M-SEARCH * HTTP/1.1")" "\r\n"
		R"(MX: 2)" "\r\n"
		R"(HOST: {}:{})" "\r\n"
		R"(MAN: "ssdp:discover")" "\r\n"
		R"(ST: urn:schemas-upnp-org:{}:{}:{})" "\r\n"
	};

	std::vector<std::uint8_t> buffer;
	buffer.reserve(4096);
	std::format_to(std::back_inserter(buffer), request, mcast_ipv4_addr, mcast_dest_port, type, subtype, version);
	return buffer;
}

asio::awaitable<void> SSDP::start_ssdp_search(const std::string_view type,
                                              const std::string_view subtype,
                                              const int version) {
	auto buffer = build_ssdp_request(type, subtype, version);
	const auto result = co_await transport_.send(buffer);

	if(!result) {
		handler_(std::unexpected(ErrorCode::network_failure));
	}
}

asio::awaitable<LocateResult> SSDP::locate_gateways(use_awaitable_t) {
	// this isn't great but it'd need a design rethink
	auto buffer = build_ssdp_request("service", "WANIPConnection", 1);
	auto result = co_await transport_.send(buffer);

	if(!result) {
		co_return std::unexpected(ErrorCode::network_failure);
	}

	buffer = build_ssdp_request("service", "WANIPConnection", 2);
	result = co_await transport_.send(buffer);

	if(!result) {
		co_return std::unexpected(ErrorCode::network_failure);
	}

	auto recv_res = co_await transport_.receive();

	if(!recv_res) {
		if(recv_res.error() == asio::error::operation_aborted) {
			co_return std::unexpected(ErrorCode::operation_aborted);
		} else {
			co_return std::unexpected(ErrorCode::network_failure);
		}
	}

	auto proc_res = validate_message(*recv_res);

	if(!proc_res) {
		co_return std::unexpected(proc_res);
	}

	co_return build_locate_result(*recv_res);
}

std::future<LocateResult> SSDP::locate_gateways(use_future_t) {
	auto promise = std::make_shared<std::promise<LocateResult>>();
	auto future = promise->get_future();
	
	locate_gateways([&, promise](LocateResult result) {
		promise->set_value(std::move(result));
		return false;
	});

	return future;
}

void SSDP::locate_gateways(LocateHandler&& handler) {
	handler_ = handler;

	asio::post(strand_, [&]() {
		asio::co_spawn(ctx_, start_ssdp_search("service", "WANIPConnection", 1), asio::detached);
		asio::co_spawn(ctx_, start_ssdp_search("service", "WANIPConnection", 2), asio::detached);
	});
}

void SSDP::search(const std::string& type, const std::string& subtype,
                  const int version, LocateHandler&& handler) {
	handler_ = handler;

	asio::post(strand_, [&, type, subtype, version]() {
		asio::co_spawn(ctx_, start_ssdp_search(type, subtype, version), asio::detached);
	});
}

}; // upnp, ports, ember