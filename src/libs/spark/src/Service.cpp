/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/Service.h>
#include <spark/MessageHandler.h>
#include <spark/NetworkSession.h>
#include <spark/Listener.h>
#include <shared/FilterTypes.h>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/optional.hpp>
#include <chrono>
#include <functional>
#include <type_traits>

using namespace std::chrono_literals;

namespace ember { namespace spark {

namespace bai = boost::asio::ip;

Service::Service(std::string description, boost::asio::io_service& service, const std::string& interface,
                 std::uint16_t port, log::Logger* logger)
                 : service_(service), logger_(logger),
                   listener_(service, interface, port, sessions_, dispatcher_, services_, link_, logger),
                   hb_service_(service_, this, logger), 
                   track_service_(service_, logger),
                   link_ { boost::uuids::random_generator()(), std::move(description) } {
	dispatcher_.register_handler(&hb_service_, messaging::Service::CORE_HEARTBEAT, EventDispatcher::Mode::BOTH);
	dispatcher_.register_handler(&track_service_, messaging::Service::CORE_HEARTBEAT, EventDispatcher::Mode::CLIENT);
}

void Service::shutdown() {
	LOG_DEBUG_FILTER(logger_, LF_SPARK) << "[spark] Service shutting down..." << LOG_ASYNC;
	track_service_.shutdown();
	hb_service_.shutdown();
	listener_.shutdown();
	sessions_.stop_all();
}

void Service::start_session(boost::asio::ip::tcp::socket socket) {
	LOG_TRACE_FILTER(logger_, LF_SPARK) << __func__ << LOG_ASYNC;

	MessageHandler m_handler(dispatcher_, services_, link_, true, logger_);
	auto session = std::make_shared<NetworkSession>(sessions_, std::move(socket), m_handler, logger_);
	sessions_.start(session);
}

void Service::do_connect(const std::string& host, std::uint16_t port) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;
	bai::tcp::resolver resolver(service_);
	auto port_str = std::to_string(port);
	auto endpoint_it = resolver.resolve({ host, port_str });
	auto socket = std::make_shared<boost::asio::ip::tcp::socket>(service_);

	boost::asio::async_connect(*socket, endpoint_it,
		[this, host, port, socket](boost::system::error_code ec, bai::tcp::resolver::iterator it) {
			if(!ec) {
				start_session(std::move(*socket));
			}

			LOG_DEBUG_FILTER(logger_, LF_SPARK)
				<< "[spark] " << (ec? "Unable to establish" : "Established")
				<< " connection to " << host << ":" << port << LOG_ASYNC;
		}
	);
}

void Service::connect(const std::string& host, std::uint16_t port) {
	LOG_TRACE_FILTER(logger_, LF_SPARK) << __func__ << LOG_ASYNC;
	do_connect(host, port);
}

auto Service::send(const Link& link, std::uint16_t opcode, BufferHandle fbb) const -> Result {
	auto net = link.net.lock();

	if(!net) {
		return Result::LINK_GONE;
	}

	net->write(fbb);
	return Result::OK;
}

auto Service::send(const Link& link, std::uint16_t opcode, BufferHandle fbb,
                   TrackingHandler callback) -> Result {
	auto net = link.net.lock();

	if(!net) {
		return Result::LINK_GONE;
	}

	auto uuid = generate_uuid();
	track_service_.register_tracked(link, uuid, callback, 5s);
	net->write(fbb);
	return Result::OK;
}

auto Service::send(const Link& link, std::uint16_t opcode, BufferHandle fbb,
                   const Beacon& token) const -> Result {
	auto net = link.net.lock();

	if(!net) {
		return Result::LINK_GONE;
	}

	net->write(fbb);
	return Result::OK;
}

auto Service::send(const Link& link, std::uint16_t opcode, BufferHandle fbb,
                   const Beacon& token, TrackingHandler callback) -> Result {
	auto net = link.net.lock();

	if(!net) {
		return Result::LINK_GONE;
	}

	track_service_.register_tracked(link, token, callback, 5s);
	net->write(fbb);
	return Result::OK;
}

void Service::broadcast(messaging::Service service, ServicesMap::Mode mode,
                        BufferHandle fbb) const {
	LOG_TRACE_FILTER(logger_, LF_SPARK) << __func__ << LOG_ASYNC;
	const auto& links = services_.peer_services(service, mode);

	for(const auto& link : links) {
		/* The weak_ptr should never fail to lock as the link will be removed from the
		   services map before the network session shared_ptr goes out of scope */
		auto shared_net = link.net.lock();
		
		if(shared_net) {
			shared_net->write(fbb);
		} else {
			LOG_WARN_FILTER(logger_, LF_SPARK) << "[spark] Unable to lock weak_ptr!" << LOG_ASYNC;
		}
	}
}

EventDispatcher* Service::dispatcher() {
	return &dispatcher_;
}

Service::~Service() {
	dispatcher_.remove_handler(&hb_service_);
	dispatcher_.remove_handler(&track_service_);
}

}} // spark, ember