/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Service.h"
#include <shared/util/EnumHelper.h>
#include <flatbuffers/flatbuffers.h>

namespace em = ember::messaging;

namespace ember {

Service::Service(Sessions& sessions, spark::Service& spark, spark::ServiceDiscovery& discovery,
                 log::Logger* logger)
                 : sessions_(sessions), spark_(spark), discovery_(discovery), logger_(logger) {
	REGISTER(em::account::Opcode::CMSG_ACCOUNT_LOOKUP, em::account::LookupID, Service::account_lookup);
	REGISTER(em::account::Opcode::CMSG_SESSION_LOOKUP, em::account::SessionLookup, Service::locate_session);
	REGISTER(em::account::Opcode::CMSG_REGISTER_SESSION, em::account::RegisterSession, Service::register_session);

	spark_.dispatcher()->register_handler(this, em::Service::ACCOUNT, spark::EventDispatcher::Mode::SERVER);
	discovery_.register_service(em::Service::ACCOUNT);
}

Service::~Service() {
	discovery_.remove_service(em::Service::ACCOUNT);
	spark_.dispatcher()->remove_handler(this);
}

void Service::on_message(const spark::Link& link, const spark::Message& message) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto handler = handlers_.find(static_cast<messaging::account::Opcode>(message.opcode));

	if(handler == handlers_.end()) {
		LOG_WARN_FILTER(logger_, LF_SPARK)
			<< "Unhandled message received from "
			<< link.description << LOG_ASYNC;
		return;
	}

	if(!handler->second.verify(message)) {
		LOG_WARN_FILTER(logger_, LF_SPARK)
			<< "[spark] Bad message received from "
			<< link.description << LOG_ASYNC;
		return;
	}

	handler->second.handle(link, message);
}

void Service::on_link_up(const spark::Link& link) {
	LOG_INFO(logger_) << "Link up: " << link.description << LOG_ASYNC;
}

void Service::on_link_down(const spark::Link& link) {
	LOG_INFO(logger_) << "Link down: " << link.description << LOG_ASYNC;
}

void Service::register_session(const spark::Link& link, const spark::Message& message) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto msg = flatbuffers::GetRoot<em::account::RegisterSession>(message.data);
	auto status = em::account::Status::OK;
	
	if(msg->key() && msg->account_id()) {
		Botan::BigInt key(msg->key()->data(), msg->key()->size());

		if(!sessions_.register_session(msg->account_id(), key)) {
			status = em::account::Status::ALREADY_LOGGED_IN;
		}
	} else {
		status = em::account::Status::ILLFORMED_MESSAGE;
	}

	send_register_reply(link, status, message.token);
}

void Service::locate_session(const spark::Link& link, const spark::Message& message) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto msg = flatbuffers::GetRoot<em::account::SessionLookup>(message.data);
	auto session = std::optional<Botan::BigInt>();
	
	if(msg->account_id()) {
		session = sessions_.lookup_session(msg->account_id());
	}

	send_locate_reply(link, session, message.token);
}

void Service::send_register_reply(const spark::Link& link, em::account::Status status,
                                  const spark::Beacon& token) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto opcode = util::enum_value(em::account::Opcode::SMSG_REGISTER_SESSION);
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	em::account::SessionResponseBuilder rb(*fbb);
	rb.add_status(status);
	fbb->Finish(rb.Finish()); // check this

	spark_.send(link, opcode, fbb);
}

void Service::account_lookup(const spark::Link& link, const spark::Message& message) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto opcode = util::enum_value(em::account::Opcode::SMSG_ACCOUNT_LOOKUP);
	auto msg = flatbuffers::GetRoot<em::account::LookupID>(message.data);
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();

	em::account::LookupIDResponseBuilder klb(*fbb);
	klb.add_status(em::account::Status::OK);
	klb.add_account_id(1); // todo
	auto data_offset = klb.Finish();
	fbb->Finish(klb.Finish());

	spark_.send(link, opcode, fbb, message.token);
}

void Service::send_locate_reply(const spark::Link& link, const std::optional<Botan::BigInt>& key,
                                const spark::Beacon& token) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto opcode = util::enum_value(em::account::Opcode::SMSG_SESSION_LOOKUP);
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();

	if(key) {
		auto encoded_key = Botan::BigInt::encode(*key);
		auto offset = fbb->CreateVector(encoded_key.data(), encoded_key.size());
		em::account::SessionResponseBuilder klb(*fbb);
		klb.add_key(offset);
		klb.add_status(em::account::Status::OK);
		fbb->Finish(klb.Finish());
	} else {
		em::account::SessionResponseBuilder klb(*fbb);
		klb.add_status(em::account::Status::SESSION_NOT_FOUND);
		fbb->Finish(klb.Finish());
	}

	spark_.send(link, opcode, fbb, token);
}

} // ember