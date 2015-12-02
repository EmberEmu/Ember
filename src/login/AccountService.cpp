/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "AccountService.h"
#include <boost/uuid/uuid.hpp>

namespace em = ember::messaging;

namespace ember {

AccountService::AccountService(spark::Service& spark, spark::ServiceDiscovery& s_disc, log::Logger* logger)
                               : spark_(spark), s_disc_(s_disc), logger_(logger) {
	spark_.dispatcher()->register_handler(this, em::Service::Account, spark::EventDispatcher::Mode::CLIENT);
	listener_ = std::move(s_disc_.listener(messaging::Service::Account,
	                      std::bind(&AccountService::service_located, this, std::placeholders::_1)));
	listener_->search();
}

AccountService::~AccountService() {
	spark_.dispatcher()->remove_handler(this);
}

void AccountService::handle_message(const spark::Link& link, const em::MessageRoot* root) {
	// we only care about tracked messages at the moment
	LOG_DEBUG(logger_) << "Session service received unhandled message" << LOG_ASYNC;
}

void AccountService::handle_link_event(const spark::Link& link, spark::LinkState event) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	switch(event) {
		case spark::LinkState::LINK_UP:
			LOG_INFO(logger_) << "Link to account server established" << LOG_ASYNC;
			link_ = link;
			break;
		case spark::LinkState::LINK_DOWN:
			LOG_INFO(logger_) << "Link to account server closed" << LOG_ASYNC;
			break;
	}
}

void AccountService::service_located(const messaging::multicast::LocateAnswer* message) {
	LOG_DEBUG(logger_) << "Located account service at " << message->ip()->str() << LOG_ASYNC; // todo
	spark_.connect(message->ip()->str(), message->port());
}

void AccountService::handle_register_reply(const spark::Link& link, const boost::uuids::uuid& uuid,
                                           boost::optional<const em::MessageRoot*> root, RegisterCB cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!root || (*root)->data_type() != messaging::Data::Response) {
		cb(em::account::Status::SERVER_LINK_ERROR);
		return;
	}

	auto message = static_cast<const em::account::Response*>((*root)->data());
	cb(message->status());
}

void AccountService::handle_locate_reply(const spark::Link& link, const boost::uuids::uuid& uuid,
                                         boost::optional<const messaging::MessageRoot*> root, LocateCB cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!root || (*root)->data_type() != messaging::Data::KeyLookupResp) {
		cb(em::account::Status::SERVER_LINK_ERROR, 0);
		return;
	}

	auto message = static_cast<const messaging::account::KeyLookupResp*>((*root)->data());
	auto key = message->key();

	if(!key) {
		cb(message->status(), 0);
		return;
	}

	cb(message->status(), Botan::BigInt::decode(key->data(), key->size()));
}

void AccountService::locate_session(std::uint32_t account_id, LocateCB cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto uuid = generate_uuid();
	auto uuid_bytes = fbb->CreateVector(uuid.begin(), uuid.static_size());
	auto msg = messaging::CreateMessageRoot(*fbb, messaging::Service::Account, uuid_bytes, 0,
		em::Data::KeyLookup, em::account::CreateKeyLookup(*fbb, account_id).Union());
	fbb->Finish(msg);

	auto track_cb = std::bind(&AccountService::handle_locate_reply, this, std::placeholders::_1,
	                          std::placeholders::_2, std::placeholders::_3, cb);

	if(spark_.send_tracked(link_, uuid, fbb, track_cb) != spark::Service::Result::OK) {
		cb(em::account::Status::SERVER_LINK_ERROR, 0);
	}
}


void AccountService::register_session(std::uint32_t account_id, const srp6::SessionKey& key, RegisterCB cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto uuid = generate_uuid();
	auto uuid_bytes = fbb->CreateVector(uuid.begin(), uuid.static_size());
	auto f_key = fbb->CreateVector(key.t.begin(), key.t.size());
	auto msg = messaging::CreateMessageRoot(*fbb, messaging::Service::Account, uuid_bytes, 0,
		em::Data::RegisterKey, em::account::CreateRegisterKey(*fbb, account_id, f_key).Union());
	fbb->Finish(msg);

	auto track_cb = std::bind(&AccountService::handle_register_reply, this, std::placeholders::_1,
	                          std::placeholders::_2, std::placeholders::_3, cb);
	
	if(spark_.send_tracked(link_, uuid, fbb, track_cb) != spark::Service::Result::OK) {
		cb(em::account::Status::SERVER_LINK_ERROR);
	}
}

} // ember