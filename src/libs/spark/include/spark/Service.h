/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spark/Common.h>
#include <spark/ServiceDiscovery.h>
#include <spark/HeartbeatService.h>
#include <spark/TrackingService.h>
#include <spark/ServicesMap.h>
#include <spark/EventDispatcher.h>
#include <spark/Link.h>
#include <spark/SessionManager.h>
#include <spark/NetworkSession.h>
#include <spark/Listener.h>
#include <logger/Logger.h>
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <flatbuffers/flatbuffers.h>
#include <memory>
#include <string>
#include <cstdint>

namespace ember { namespace spark {

class Service {
	typedef std::shared_ptr<flatbuffers::FlatBufferBuilder> BufferHandler;

	boost::asio::io_service& service_;
	boost::asio::signal_set signals_;
	boost::asio::ip::tcp::socket socket_;

	Link link_;
	Listener listener_;
	EventDispatcher dispatcher_;
	ServicesMap services_;
	SessionManager sessions_;
	HeartbeatService hb_service_;
	TrackingService track_service_;

	log::Logger* logger_;
	log::Filter filter_;
	
	void do_connect(const std::string& host, std::uint16_t port);
	void start_session(boost::asio::ip::tcp::socket socket);
	void default_handler(const Link& link, const messaging::MessageRoot* message);
	void default_link_state_handler(const Link& link, LinkState state);
	void initiate_handshake(NetworkSession* session);

public:
	enum class Result { OK, LINK_GONE };

	Service(std::string description, boost::asio::io_service& service, const std::string& interface,
	        std::uint16_t port, log::Logger* logger, log::Filter filter);
	~Service();

	EventDispatcher* dispatcher();
	void connect(const std::string& host, std::uint16_t port);
	Result send(const Link& link, BufferHandler fbb) const;
	Result send_tracked(const Link& link, boost::uuids::uuid id,
	                    BufferHandler fbb, TrackingHandler callback);
	Result broadcast(messaging::Service service, ServicesMap::Mode mode, BufferHandler fbb) const;
	void shutdown();
};

}} // spark, ember