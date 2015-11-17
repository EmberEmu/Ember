/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spark/MessageHandler.h>
#include <spark/HandlerMap.h>
#include <spark/NetworkSession.h>
#include <spark/Utility.h>
#include <spark/temp/MessageRoot_generated.h>
#include <spark/temp/Core_generated.h>
#include <spark/temp/ServiceTypes_generated.h>
#include <flatbuffers/flatbuffers.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace ember { namespace spark {

MessageHandler::MessageHandler(const HandlerMap& handlers, const Link& link, bool initiator,
                               log::Logger* logger, log::Filter filter)
                               : handlers_(handlers), self_(link), initiator_(initiator),
                                 logger_(logger), filter_(filter) { }


void MessageHandler::send_negotiation(NetworkSession& net) {
	LOG_TRACE_FILTER(logger_, filter_) << __func__ << LOG_ASYNC;

	auto services = handlers_.outbound_services();
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	std::array<uint8_t, 2> test {1, 2};
	auto in = fbb->CreateVector(test.data(), test.size());
	auto out = fbb->CreateVector(test.data(), test.size());

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

	//std::vector<messaging::Service> in(protocols->proto_in()->begin(), protocols->proto_in()->end());

	if(!initiator_) {
		send_negotiation(net);
	}

	state_ = State::FORWARDING;
	return true;
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

}} // spark, ember