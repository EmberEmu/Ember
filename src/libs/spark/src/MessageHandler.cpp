/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Core_generated.h"
#include <spark/Service.h>
#include <spark/MessageHandler.h>
#include <spark/EventDispatcher.h>
#include <spark/NetworkSession.h>
#include <spark/Utility.h>
#include <shared/FilterTypes.h>
#include <flatbuffers/flatbuffers.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <algorithm>
#include <vector>

namespace ember { namespace spark {

MessageHandler::MessageHandler(const EventDispatcher& dispatcher, ServicesMap& services, const Link& link,
                               bool initiator, log::Logger* logger)
                               : dispatcher_(dispatcher), self_(link), initiator_(initiator),
                                 logger_(logger), services_(services), peer_{} { }


void MessageHandler::send_negotiation(NetworkSession& net) {
	LOG_TRACE_FILTER(logger_, LF_SPARK) << __func__ << LOG_ASYNC;

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto in = fbb->CreateVector(detail::services_to_underlying(dispatcher_.services(EventDispatcher::Mode::SERVER)));
	auto out = fbb->CreateVector(detail::services_to_underlying(dispatcher_.services(EventDispatcher::Mode::CLIENT)));
	auto msg = messaging::core::CreateNegotiate(*fbb, in, out);
	fbb->Finish(msg);
	net.write(fbb);
}

void MessageHandler::send_banner(NetworkSession& net) {
	LOG_TRACE_FILTER(logger_, LF_SPARK) << __func__ << LOG_ASYNC;

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto desc = fbb->CreateString(self_.description);
	auto uuid = fbb->CreateVector(self_.uuid.begin(), self_.uuid.size());
	auto msg = messaging::core::CreateBanner(*fbb, desc, uuid);
	fbb->Finish(msg);
	net.write(fbb);
}

bool MessageHandler::establish_link(NetworkSession& net, const Message& message) {
	LOG_TRACE_FILTER(logger_, LF_SPARK) << __func__ << LOG_ASYNC;

	if(static_cast<messaging::core::Opcode>(message.opcode) != messaging::core::Opcode::MSG_BANNER) {
		LOG_WARN_FILTER(logger_, LF_SPARK)
			<< "[spark] Link failed, peer did not send banner: "
			<< net.remote_host() << LOG_ASYNC;
		return false;
	}

	auto banner = flatbuffers::GetRoot<messaging::core::Banner>(message.data);

	if(!banner->server_uuid() || banner->server_uuid()->size() != boost::uuids::uuid::static_size()
	   || !banner->description()) {
		LOG_WARN_FILTER(logger_, LF_SPARK)
			<< "[spark] Link failed, incompatible banner: "
			<< net.remote_host() << LOG_ASYNC;
		return false;
	}

	std::copy(banner->server_uuid()->begin(), banner->server_uuid()->end(), peer_.uuid.data);
	peer_.description = banner->description()->str();
	peer_.net = std::weak_ptr<NetworkSession>(net.shared_from_this());

	LOG_TRACE_FILTER(logger_, LF_SPARK)
		<< "[spark] Peer banner: " << peer_.description << ":"
		<< boost::uuids::to_string(peer_.uuid) << LOG_ASYNC;

	if(initiator_) {
		send_negotiation(net);
	} else {
		send_banner(net);
	}

	state_ = State::NEGOTIATING;
	return true;
}

bool MessageHandler::negotiate_protocols(NetworkSession& net, const Message& message) {
	LOG_TRACE_FILTER(logger_, LF_SPARK) << __func__ << LOG_ASYNC;

	if(static_cast<messaging::core::Opcode>(message.opcode) != messaging::core::Opcode::MSG_NEGOTIATE) {
		LOG_WARN_FILTER(logger_, LF_SPARK)
			<< "[spark] Link failed, peer did not negotiate: "
			<< net.remote_host() << LOG_ASYNC;
		return false;
	}

	auto protocols = flatbuffers::GetRoot<messaging::core::Negotiate>(message.data);
	
	if(!protocols->proto_in() || !protocols->proto_out()) {
		LOG_WARN_FILTER(logger_, LF_SPARK)
			<< "[spark] Link failed, incompatible negotiation: "
			<< net.remote_host() << LOG_ASYNC;
		return false;
	}

	// vectors of enums are very annoying to use in FlatBuffers :(
	std::vector<std::underlying_type<messaging::Service>::type>
		remote_servers(protocols->proto_in()->begin(), protocols->proto_in()->end());
	std::vector<std::underlying_type<messaging::Service>::type>
		remote_clients(protocols->proto_out()->begin(), protocols->proto_out()->end());
	auto our_servers = detail::services_to_underlying(dispatcher_.services(EventDispatcher::Mode::SERVER));
	auto our_clients = detail::services_to_underlying(dispatcher_.services(EventDispatcher::Mode::CLIENT));

	std::sort(our_servers.begin(), our_servers.end());
	std::sort(our_clients.begin(), our_clients.end());
	std::sort(remote_servers.begin(), remote_servers.end());
	std::sort(remote_clients.begin(), remote_clients.end());

	// match our clients to their servers
	std::set_intersection(our_clients.begin(), our_clients.end(),
	                      remote_servers.begin(), remote_servers.end(),
	                      std::inserter(matches_, matches_.begin()));

	// match their clients to our servers
	std::set_intersection(remote_clients.begin(), remote_clients.end(),
	                      our_servers.begin(), our_servers.end(),
	                      std::inserter(matches_, matches_.begin()));

	// we don't care about matches for core service
	auto matches = std::count_if(matches_.begin(), matches_.end(),
		[](auto service) { // todo, get rid of this?
			return service != static_cast<std::underlying_type<messaging::Service>::type>(messaging::Service::CORE_HEARTBEAT)
				&& service != static_cast<std::underlying_type<messaging::Service>::type>(messaging::Service::CORE_TRACKING);
		}
	);

	if(!matches) {
		LOG_DEBUG_FILTER(logger_, LF_SPARK)
			<< "[spark] Peer did not match any supported protocols: "
			<< net.remote_host() << LOG_ASYNC;
		return false;
	}

	if(!initiator_) {
		send_negotiation(net);
	}

	LOG_INFO_FILTER(logger_, LF_SPARK)
		<< "[spark] Established link: " << peer_.description << ":"
		<< boost::uuids::to_string(peer_.uuid) << LOG_ASYNC;

	// register peer's services to allow for broadcasting
	for(auto& service : remote_clients) {
		services_.register_peer_service(peer_, static_cast<messaging::Service>(service),
		                                ServicesMap::Mode::CLIENT);
	}

	for(auto& service : remote_servers) {
		services_.register_peer_service(peer_, static_cast<messaging::Service>(service),
										ServicesMap::Mode::SERVER);
	}

	// send the 'link up' event handlers for all matched services
	for(auto& service : matches_) {
		dispatcher_.notify_link_up(static_cast<messaging::Service>(service), peer_);
	}

	state_ = State::DISPATCHING;
	return true;
}

void MessageHandler::dispatch_message(const Message& message) {
	// if there's a tracking UUID set in the message, route it through the tracking service
	if(message.token) {
		dispatcher_.dispatch_message(messaging::Service::CORE_TRACKING, peer_, message);
	} else {
		dispatcher_.dispatch_message(message.service, peer_, message);
	}
}

bool MessageHandler::handle_message(NetworkSession& net, const messaging::core::Header* header,
                                    const std::uint8_t* data, std::uint32_t size) {
	boost::optional<ResponseToken> token;

	if(header->uuid()) {
		boost::uuids::uuid uuid;
		std::copy(header->uuid()->begin(), header->uuid()->end(), uuid.data);
		token = std::move(uuid);
	}

	const Message message { header->service(), header->opcode(), size, data, token };

	switch(state_) {
		case State::HANDSHAKING:
			return establish_link(net, message);
		case State::NEGOTIATING:
			return negotiate_protocols(net, message);
		case State::DISPATCHING:
			dispatch_message(message);
			return true;
	}

	return false;
}

void MessageHandler::start(NetworkSession& net) {
	LOG_TRACE_FILTER(logger_, LF_SPARK) << __func__ << LOG_ASYNC;
	
	if(initiator_) {
		send_banner(net);
	}
}

MessageHandler::~MessageHandler() {
	if(state_ != State::DISPATCHING) {
		return;
	}

	services_.remove_peer(peer_);

	// send the link down event to any handlers
	for(auto& service : matches_) {
		dispatcher_.notify_link_down(static_cast<messaging::Service>(service), peer_);
	}
}

}} // spark, ember