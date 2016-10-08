/*
 * Copyright (c) 2015, 2016 Ember
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
#include <logger/Logging.h>
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <flatbuffers/flatbuffers.h>
#include <memory>
#include <string>
#include <cstdint>

namespace ember { namespace spark {

class Service final {
	typedef std::shared_ptr<flatbuffers::FlatBufferBuilder> BufferHandle;

	boost::asio::io_service& service_;

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
	void start_session(boost::asio::ip::tcp::socket socket);

public:
	enum class Result { OK, LINK_GONE };

	Service(std::string description, boost::asio::io_service& service,
	        const std::string& interface, std::uint16_t port, log::Logger* logger);
	~Service();

	EventDispatcher* dispatcher();
	void connect(const std::string& host, std::uint16_t port);

	Result send(const Link& link, std::uint16_t opcode, BufferHandle fbb) const;
	Result send(const Link& link, std::uint16_t opcode, BufferHandle fbb,
	            const ResponseToken& token) const;

	Result send(const Link& link, std::uint16_t opcode, BufferHandle fbb,
	            const ResponseToken& token, TrackingHandler callback);
	Result send(const Link& link, std::uint16_t opcode, BufferHandle fbb,
	            TrackingHandler callback);

	void broadcast(messaging::Service service, ServicesMap::Mode mode, BufferHandle fbb) const;
	void shutdown();

	template<typename MessageType>
	[[nodiscard]] static bool verify(const Message& message) {
		flatbuffers::Verifier verifier(message.data, message.size);
		auto message = flatbuffers::GetRoot<MessageType>(message.data);
		return message->Verify(verifier);
	}
};

}} // spark, ember