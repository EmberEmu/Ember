/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "AccountService.h"
#include <spark/temp/Account_generated.h>
#include <boost/uuid/uuid.hpp>
#include <functional>

namespace em = ember::messaging;

namespace ember {

AccountService::AccountService(spark::Service& spark, spark::ServiceDiscovery& s_disc, log::Logger* logger)
                               : spark_(spark), s_disc_(s_disc), logger_(logger) {
	spark_.dispatcher()->register_handler(this, messaging::Service::Account, spark::EventDispatcher::Mode::CLIENT);

	listener_ = std::move(s_disc_.listener(messaging::Service::Account,
	                      std::bind(&AccountService::service_located, this, std::placeholders::_1)));
	listener_->search();
}

AccountService::~AccountService() {
	spark_.dispatcher()->remove_handler(this);
}

void AccountService::handle_message(const spark::Link& link, const em::MessageRoot* msg) {
	// we only care about tracked messages at the moment
	LOG_DEBUG(logger_) << "Session service received unhandled message" << LOG_ASYNC;
}

void AccountService::handle_link_event(const spark::Link& link, spark::LinkState event) {
	switch(event) {
		case spark::LinkState::LINK_UP:
			LOG_INFO(logger_) << "Link to account server established" << LOG_ASYNC;
			link_ = link;
			break;
		case spark::LinkState::LINK_DOWN:
			LOG_WARN(logger_) << "Link to account server lost" << LOG_ASYNC;
			break;
	}
}

void AccountService::service_located(const messaging::multicast::LocateAnswer* message) {
	LOG_DEBUG(logger_) << "Found service" << LOG_ASYNC;
	spark_.connect(message->ip()->str(), message->port());
}

void AccountService::handle_register_reply(const spark::Link& link, const boost::uuids::uuid& uuid,
                                           boost::optional<const em::MessageRoot*> opt_msg, RegisterCB cb) const {
	if(!opt_msg || (*opt_msg)->data_type() != messaging::Data::Response) {
		cb(Result::SERVER_LINK_FAILURE);
		return;
	}

	auto message = static_cast<const em::account::Response*>((*opt_msg)->data());

	if(message->status() == em::account::Status::OK) {
		cb(Result::OK);
	} else {
		cb(Result::SERVER_LINK_FAILURE);
	}
}

void AccountService::handle_locate_reply(const spark::Link& link, const boost::uuids::uuid& uuid,
                                         boost::optional<const messaging::MessageRoot*> opt_msg, LocateCB cb) const {
	if(!opt_msg || (*opt_msg)->data_type() != messaging::Data::KeyLookup) {
		cb(Result::SERVER_LINK_FAILURE, Botan::BigInt());
		return;
	}

	auto message = static_cast<const messaging::account::KeyLookupResp*>((*opt_msg)->data());
	auto key = message->key();

	if(!key) {
		cb(Result::OK, boost::optional<Botan::BigInt>());
		return;
	}

	cb(Result::OK, Botan::BigInt(key->data(), key->size()));
}

void AccountService::locate_session(std::uint32_t account_id, LocateCB cb) const {
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto uuid = generate_uuid();
	auto uuid_bytes = fbb->CreateVector(uuid.begin(), uuid.static_size());
	auto msg = messaging::CreateMessageRoot(*fbb, messaging::Service::Account, uuid_bytes, 1,
		em::Data::KeyLookup, em::account::CreateKeyLookup(*fbb, account_id).Union());
	fbb->Finish(msg);
	auto track_cb = std::bind(&AccountService::handle_locate_reply, this, std::placeholders::_1,
	                          std::placeholders::_2, std::placeholders::_3, cb);

	if(spark_.send_tracked(link_, uuid, fbb, track_cb) != spark::Service::Result::OK) {
		cb(Result::SERVER_LINK_FAILURE, Botan::BigInt());
	}
}


void AccountService::register_session(std::uint32_t account_id, const srp6::SessionKey& key, RegisterCB cb) const {
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto uuid = generate_uuid();
	auto uuid_bytes = fbb->CreateVector(uuid.begin(), uuid.static_size());
	auto f_key = fbb->CreateVector(key.t.begin(), key.t.size());
	auto msg = messaging::CreateMessageRoot(*fbb, messaging::Service::Account, uuid_bytes, 1,
		em::Data::RegisterKey, em::account::CreateRegisterKey(*fbb, account_id, f_key).Union());
	fbb->Finish(msg);

	auto track_cb = std::bind(&AccountService::handle_register_reply, this, std::placeholders::_1,
					          std::placeholders::_2, std::placeholders::_3, cb);
	
	if(spark_.send_tracked(link_, uuid, fbb, track_cb) != spark::Service::Result::OK) {
		cb(Result::SERVER_LINK_FAILURE);
	}
}

} //ember