/*
 * Copyright (c) 2015 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Services_generated.h"
#include "ServiceDiscovery.h"
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
#include <shared/FilterTypes.h>
#include <logger/LoggerFwd.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <flatbuffers/flatbuffers.h>
#include <memory>
#include <string>
#include <cstdint>

namespace ember::spark::inline v1 {

class Service final {
	using BufferHandle = std::shared_ptr<flatbuffers::FlatBufferBuilder>;

	boost::asio::io_context& service_;

	Link link_;
	EventDispatcher dispatcher_;
	ServicesMap services_;
	SessionManager sessions_;
	HeartbeatService hb_service_;
	TrackingService track_service_;
	Listener listener_;
	boost::uuids::random_generator generate_uuid;

	log::Logger* logger_;
	
	void do_connect(const std::string& host, std::uint16_t port);
	void start_session(boost::asio::ip::tcp::socket socket, boost::asio::ip::tcp::endpoint ep);

public:
	enum class Result { OK, LINK_GONE };

	Service(std::string description, boost::asio::io_context& service,
	        const std::string& interface, std::uint16_t port, log::Logger* logger);
	~Service();

	EventDispatcher* dispatcher();
	void connect(const std::string& host, std::uint16_t port);

	Result send(const Link& link, std::uint16_t opcode, BufferHandle fbb) const;
	Result send(const Link& link, std::uint16_t opcode, BufferHandle fbb,
	            const Beacon& token);

	Result send(const Link& link, std::uint16_t opcode, BufferHandle fbb,
	            const Beacon& token, TrackingHandler callback);
	Result send(const Link& link, std::uint16_t opcode, BufferHandle fbb,
	            TrackingHandler callback);

	void broadcast(messaging::Service service, ServicesMap::Mode mode, BufferHandle fbb) const;
	void shutdown();

	template<typename MessageType>
	/*[[nodiscard]] todo*/ static bool verify(const Message& message) {
		flatbuffers::Verifier verifier(message.data, message.size);
		auto root = flatbuffers::GetRoot<MessageType>(message.data);
		return root->Verify(verifier);
	}
};

} // spark, ember