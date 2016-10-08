/*
 * Copyright (c) 2015, 2016 Ember
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
	spark_.dispatcher()->register_handler(this, em::Service::ACCOUNT, spark::EventDispatcher::Mode::CLIENT);
	listener_ = std::move(s_disc_.listener(messaging::Service::ACCOUNT,
	                      std::bind(&AccountService::service_located, this, std::placeholders::_1)));
	listener_->search();
}

AccountService::~AccountService() {
	spark_.dispatcher()->remove_handler(this);
}

void AccountService::on_link_up(const spark::Link& link) {
	LOG_INFO(logger_) << "Link up: " << link.description << LOG_ASYNC;
	link_ = link;
}

void AccountService::on_link_down(const spark::Link& link) {
	LOG_INFO(logger_) << "Link down: " << link.description << LOG_ASYNC;
}

void AccountService::on_message(const spark::Link& link, const spark::ResponseToken& token,
                                std::uint16_t opcode, const std::uint8_t* data) {
	// we only care about tracked messages at the moment
	LOG_DEBUG(logger_) << "Account service received unhandled message" << LOG_ASYNC;
}


void AccountService::service_located(const messaging::multicast::LocateAnswer* message) {
	LOG_DEBUG(logger_) << "Located account service at " << message->ip()->str() 
	                   << ":" << message->port() << LOG_ASYNC;
	spark_.connect(message->ip()->str(), message->port());
}

void AccountService::handle_register_reply(const spark::Link& link, const boost::uuids::uuid& uuid,
                                           boost::optional<const em::MessageRoot*> root,
                                           const RegisterCB& cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!root || (*root)->data_type() != messaging::Data::Response) {
		cb(em::account::Status::SERVER_LINK_ERROR);
		return;
	}

	auto message = static_cast<const em::account::Response*>((*root)->data());
	cb(message->status());
}

void AccountService::handle_locate_reply(const spark::Link& link, const boost::uuids::uuid& uuid,
                                         boost::optional<const messaging::MessageRoot*> root,
                                         const SessionLocateCB& cb) const {
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

void AccountService::handle_id_locate_reply(const spark::Link& link, const boost::uuids::uuid& uuid,
                                            boost::optional<const messaging::MessageRoot*> root,
                                            const IDLocateCB& cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!root || (*root)->data_type() != messaging::Data::AccountLookupResponse) {
		cb(em::account::Status::SERVER_LINK_ERROR, 0);
		return;
	}

	auto message = static_cast<const messaging::account::AccountLookupResponse*>((*root)->data());
	auto account_id = message->account_id();
	cb(em::account::Status::OK, account_id); // temp
}

void AccountService::locate_session(const std::uint32_t account_id, SessionLocateCB cb) const {
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

void AccountService::locate_account_id(const std::string& username, IDLocateCB cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto uuid = generate_uuid();
	auto uuid_bytes = fbb->CreateVector(uuid.begin(), uuid.static_size());
	auto msg = messaging::CreateMessageRoot(*fbb, messaging::Service::Account, uuid_bytes, 0,
		em::Data::AccountLookup, em::account::CreateAccountLookup(*fbb, fbb->CreateString(username)).Union());
	fbb->Finish(msg);

	auto track_cb = std::bind(&AccountService::handle_id_locate_reply, this, std::placeholders::_1,
	                          std::placeholders::_2, std::placeholders::_3, cb);

	if(spark_.send_tracked(link_, uuid, fbb, track_cb) != spark::Service::Result::OK) {
		cb(em::account::Status::SERVER_LINK_ERROR, 0);
	}
}

} // ember