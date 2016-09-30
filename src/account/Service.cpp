/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Service.h"

namespace em = ember::messaging;

namespace ember {

Service::Service(Sessions& sessions, spark::Service& spark, spark::ServiceDiscovery& discovery, log::Logger* logger)
                 : sessions_(sessions), spark_(spark), discovery_(discovery), logger_(logger) {
	spark_.dispatcher()->register_handler(this, em::Service::Account, spark::EventDispatcher::Mode::SERVER);
	discovery_.register_service(em::Service::Account);
}

Service::~Service() {
	discovery_.remove_service(em::Service::Account);
	spark_.dispatcher()->remove_handler(this);
}

void Service::handle_message(const spark::Link& link, const em::MessageRoot* msg) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	switch(msg->data_type()) {
		case em::Data::RegisterKey:
			register_session(link, msg);
			break;
		case em::Data::KeyLookup:
			locate_session(link, msg);
			break;
		case em::Data::AccountLookup:
			send_account_locate_reply(link, msg); // todo
			break;
		default:
			LOG_DEBUG(logger_) << "Service received unhandled message type" << LOG_ASYNC;
	}
}

void Service::handle_link_event(const spark::Link& link, spark::LinkState event) {
	switch(event) {
		case spark::LinkState::LINK_UP:
			LOG_DEBUG(logger_) << "Link up: " << link.description << LOG_ASYNC;
			break;
		case spark::LinkState::LINK_DOWN:
			LOG_DEBUG(logger_) << "Link down: " << link.description << LOG_ASYNC;
			break;
	}
}

void Service::register_session(const spark::Link& link, const em::MessageRoot* root) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto msg = static_cast<const em::account::RegisterKey*>(root->data());
	auto status = em::account::Status::OK;
	
	if(msg->key() && msg->account_id()) {
		Botan::BigInt key(msg->key()->data(), msg->key()->size());

		if(!sessions_.register_session(msg->account_id(), key)) {
			status = em::account::Status::ALREADY_LOGGED_IN;
		}
	} else {
		status = em::account::Status::ILLFORMED_MESSAGE;
	}

	send_register_reply(link, root, status);
}

void Service::locate_session(const spark::Link& link, const em::MessageRoot* root) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto msg = static_cast<const em::account::KeyLookup*>(root->data());
	auto session = boost::optional<Botan::BigInt>();
	
	if(msg->account_id()) {
		session = sessions_.lookup_session(msg->account_id());
	}

	send_locate_reply(link, root, session);
}

void Service::send_register_reply(const spark::Link& link, const em::MessageRoot* root,
                                  em::account::Status status) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	em::account::ResponseBuilder rb(*fbb);
	rb.add_status(status);
	auto data_offset = rb.Finish();

	flatbuffers::Offset<flatbuffers::Vector<std::uint8_t>> id;

	if(root->tracking_id()) {
		id = fbb->CreateVector(root->tracking_id()->data(), root->tracking_id()->size());
	}

	em::MessageRootBuilder mrb(*fbb);
	mrb.add_service(em::Service::Account);
	mrb.add_data_type(em::Data::Response);
	mrb.add_data(data_offset.Union());

	if(root->tracking_id()) {
		mrb.add_tracking_id(id);
		mrb.add_tracking_ttl(1);
	}

	//spark_.set_tracking_data(root, mrb, fbb.get());
	auto mloc = mrb.Finish();
	
	fbb->Finish(mloc);
	spark_.send(link, fbb);
}

void Service::send_account_locate_reply(const spark::Link& link, const em::MessageRoot* root) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto msg = static_cast<const em::account::AccountLookup*>(root->data());
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();

	em::account::AccountLookupResponseBuilder klb(*fbb);
	klb.add_status(em::account::Status::OK);
	klb.add_account_id(1); // todo
	auto data_offset = klb.Finish();

	flatbuffers::Offset<flatbuffers::Vector<std::uint8_t>> id;

	if(root->tracking_id()) {
		id = fbb->CreateVector(root->tracking_id()->data(), root->tracking_id()->size());
	}

	em::MessageRootBuilder mrb(*fbb);
	mrb.add_service(em::Service::Account);
	mrb.add_data_type(em::Data::AccountLookupResponse);
	mrb.add_data(data_offset.Union());

	if(root->tracking_id()) {
		mrb.add_tracking_id(id);
		mrb.add_tracking_ttl(1);
	}

	//spark_.set_tracking_data(root, mrb, fbb.get());
	auto mloc = mrb.Finish();

	fbb->Finish(mloc);
	spark_.send(link, fbb);

	// todo, logging
}

void Service::send_locate_reply(const spark::Link& link, const em::MessageRoot* root,
                                const boost::optional<Botan::BigInt>& key) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto msg = static_cast<const em::account::KeyLookup*>(root->data());

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();

	flatbuffers::Offset<flatbuffers::Vector<std::uint8_t>> vector_key;

	if(key) {
		auto encoded_key = Botan::BigInt::encode(*key);
		vector_key = fbb->CreateVector(encoded_key.begin(), encoded_key.size());
	}

	em::account::KeyLookupRespBuilder klb(*fbb);
	em::account::Status status;

	if(key) {
		klb.add_key(vector_key);
		status = em::account::Status::OK;
	} else {
		status = em::account::Status::SESSION_NOT_FOUND;
	}

	klb.add_status(status);
	klb.add_account_id(msg->account_id());
	auto data_offset = klb.Finish();

	flatbuffers::Offset<flatbuffers::Vector<std::uint8_t>> id;

	if(root->tracking_id()) {
		id = fbb->CreateVector(root->tracking_id()->data(), root->tracking_id()->size());
	}

	em::MessageRootBuilder mrb(*fbb);
	mrb.add_service(em::Service::Account);
	mrb.add_data_type(em::Data::KeyLookupResp);
	mrb.add_data(data_offset.Union());
	//spark_.set_tracking_data(root, mrb, fbb.get());

	if(root->tracking_id()) {
		mrb.add_tracking_id(id);
		mrb.add_tracking_ttl(1);
	}

	auto mloc = mrb.Finish();

	fbb->Finish(mloc);
	spark_.send(link, fbb);

	LOG_DEBUG(logger_) << "Session key lookup: " << msg->account_id() << " -> "
	<< em::account::EnumNameStatus(status) << LOG_ASYNC;
}

} // ember