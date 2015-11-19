/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/MessageHandler.h>
#include <spark/LinkMap.h>
#include <spark/HandlerMap.h>
#include <spark/NetworkSession.h>
#include <spark/Utility.h>
#include <spark/temp/MessageRoot_generated.h>
#include <spark/temp/Core_generated.h>
#include <spark/temp/ServiceTypes_generated.h>
#include <flatbuffers/flatbuffers.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <algorithm>

namespace ember { namespace spark {

MessageHandler::MessageHandler(const HandlerMap& handlers, LinkMap& links, const Link& link, bool initiator,
                               log::Logger* logger, log::Filter filter)
                               : handlers_(handlers), links_(links), self_(link), initiator_(initiator),
                                 logger_(logger), filter_(filter) { }


void MessageHandler::send_negotiation(NetworkSession& net) {
	LOG_TRACE_FILTER(logger_, filter_) << __func__ << LOG_ASYNC;

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto in = fbb->CreateVector(detail::services_to_underlying(handlers_.services(HandlerMap::Mode::SERVER)));
	auto out = fbb->CreateVector(detail::services_to_underlying(handlers_.services(HandlerMap::Mode::CLIENT)));

	auto msg = messaging::CreateMessageRoot(*fbb, messaging::Service::Service_Core, 0,
		messaging::Data::Data_Negotiate, messaging::CreateNegotiate(*fbb, in, out).Union());

	fbb->Finish(msg);
	net.write(fbb);
}

void MessageHandler::send_banner(NetworkSession& net) {
	LOG_TRACE_FILTER(logger_, filter_) << __func__ << LOG_ASYNC;

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto desc = fbb->CreateString(self_.description);
	auto uuid = fbb->CreateVector(self_.uuid.begin(), self_.uuid.size());

	auto msg = messaging::CreateMessageRoot(*fbb, messaging::Service::Service_Core, 0,
		messaging::Data::Data_Banner, messaging::CreateBanner(*fbb, desc, uuid).Union());

	fbb->Finish(msg);
	net.write(fbb);
}

bool MessageHandler::establish_link(NetworkSession& net, const messaging::MessageRoot* message) {
	LOG_TRACE_FILTER(logger_, filter_) << __func__ << LOG_ASYNC;

	if(message->data_type() != messaging::Data_Banner) {
		LOG_WARN_FILTER(logger_, filter_)
			<< "[spark] Link failed, peer did not send banner: "
			<< net.remote_host() << LOG_ASYNC;
		return false;
	}

	auto banner = static_cast<const messaging::Banner*>(message->data());

	if(!banner->server_uuid() || banner->server_uuid()->size() != boost::uuids::uuid::static_size()
	   || !banner->description()) {
		LOG_WARN_FILTER(logger_, filter_)
			<< "[spark] Link failed, incompatible banner: "
			<< net.remote_host() << LOG_ASYNC;
		return false;
	}

	std::copy(banner->server_uuid()->begin(), banner->server_uuid()->end(), peer_.uuid.data);
	peer_.description = banner->description()->str();

	LOG_DEBUG_FILTER(logger_, filter_)
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

bool MessageHandler::negotiate_protocols(NetworkSession& net, const messaging::MessageRoot* message) {
	LOG_TRACE_FILTER(logger_, filter_) << __func__ << LOG_ASYNC;

	if(message->data_type() != messaging::Data_Negotiate) {
		LOG_WARN_FILTER(logger_, filter_)
			<< "[spark] Link failed, peer did not negotiate: "
			<< net.remote_host() << LOG_ASYNC;
		return false;
	}

	auto protocols = static_cast<const messaging::Negotiate*>(message->data());
	
	if(!protocols->proto_in() || !protocols->proto_out()) {
		LOG_WARN_FILTER(logger_, filter_)
			<< "[spark] Link failed, incompatible negotiation: "
			<< net.remote_host() << LOG_ASYNC;
		return false;
	}

	// vectors of enums are very annoying to use in FlatBuffers :(
	std::vector<std::int32_t> remote_servers(protocols->proto_in()->begin(), protocols->proto_in()->end());
	std::vector<std::int32_t> remote_clients(protocols->proto_out()->begin(), protocols->proto_out()->end());
	auto our_servers = handlers_.services(HandlerMap::Mode::SERVER);
	auto our_clients = handlers_.services(HandlerMap::Mode::CLIENT);

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
		[](std::int32_t service) {
			return service != static_cast<std::int32_t>(messaging::Service::Service_Core);
		}
	);

	if(!matches) {
		LOG_DEBUG_FILTER(logger_, filter_)
			<< "[spark] Peer did not match any supported protocols: "
			<< net.remote_host() << LOG_ASYNC;
		return false;
	}

	if(!initiator_) {
		send_negotiation(net);
	}

	links_.register_link(peer_, net.shared_from_this());

	LOG_INFO_FILTER(logger_, filter_)
		<< "[spark] Established link: " << peer_.description << ":"
		<< boost::uuids::to_string(peer_.uuid) << LOG_ASYNC;

	// send the 'link up' event handlers for all matched services
	for(auto& service : matches_) {
		handlers_.link_state_handler(static_cast<messaging::Service>(service))(peer_, LinkState::LINK_UP);
	}

	state_ = State::FORWARDING;
	return true;
}

void MessageHandler::dispatch_message(const messaging::MessageRoot* message) {
	if(message->tracking_id()) {
		handlers_.message_handler(messaging::Service::Service_Tracking)(peer_, message);
	} else {
		handlers_.message_handler(message->service())(peer_, message);
	}
}

bool MessageHandler::handle_message(NetworkSession& net, const std::vector<std::uint8_t>& buffer) {
	LOG_TRACE_FILTER(logger_, filter_) << __func__ << LOG_ASYNC;

	flatbuffers::Verifier verifier(buffer.data(), buffer.size());

	if(!messaging::VerifyMessageRootBuffer(verifier)) {
		LOG_DEBUG_FILTER(logger_, filter_)
			<< "[spark] Message failed validation, dropping peer" << LOG_ASYNC;
		return false;
	}
	
	auto message = messaging::GetMessageRoot(buffer.data());

	switch(state_) {
		case State::HANDSHAKING:
			return establish_link(net, message);
		case State::NEGOTIATING:
			return negotiate_protocols(net, message);
		case State::FORWARDING:
			dispatch_message(message);
			return true;
	}

	return false;
}

void MessageHandler::start(NetworkSession& net) {
	LOG_TRACE_FILTER(logger_, filter_) << __func__ << LOG_ASYNC;
	
	if(initiator_) {
		send_banner(net);
	}
}

MessageHandler::~MessageHandler() {
	if(state_ != State::FORWARDING) {
		return;
	}

	// send the link down event to any handlers
	for(auto& service : matches_) {
		handlers_.link_state_handler(static_cast<messaging::Service>(service))(peer_, LinkState::LINK_DOWN);
	}

	links_.unregister_link(peer_);
}

}} // spark, ember