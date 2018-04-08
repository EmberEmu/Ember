/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "AccountService.h"
#include <shared/util/EnumHelper.h>
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

void AccountService::on_message(const spark::Link& link, const spark::Message& message) {
	LOG_WARN(logger_) << "Account service received unhandled message" << LOG_ASYNC;
}


void AccountService::service_located(const messaging::multicast::LocateResponse* message) {
	LOG_DEBUG(logger_) << "Located account service at " << message->ip()->str() 
	                   << ":" << message->port() << LOG_ASYNC;
	spark_.connect(message->ip()->str(), message->port());
}

void AccountService::handle_register_reply(const spark::Link& link,
                                           std::optional<spark::Message>& root,
                                           const RegisterCB& cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!root) {
		cb(em::account::Status::SERVER_LINK_ERROR);
		return;
	}

	auto message = flatbuffers::GetRoot<em::account::Response>(root->data);
	cb(message->status());
}

void AccountService::handle_locate_reply(const spark::Link& link,
                                         std::optional<spark::Message>& root,
                                         const SessionLocateCB& cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!root) {
		cb(em::account::Status::SERVER_LINK_ERROR, 0);
		return;
	}

	auto message = flatbuffers::GetRoot<em::account::SessionResponse>(root->data);
	auto key = message->key();

	if(!key) {
		cb(message->status(), 0);
		return;
	}

	cb(message->status(), Botan::BigInt::decode(key->data(), key->size()));
}

void AccountService::handle_id_locate_reply(const spark::Link& link,
                                            std::optional<spark::Message>& root,
                                            const IDLocateCB& cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!root) {
		cb(em::account::Status::SERVER_LINK_ERROR, 0);
		return;
	}

	auto message = flatbuffers::GetRoot<em::account::LookupIDResponse>(root->data);
	cb(em::account::Status::OK, message->account_id());
}

void AccountService::locate_session(std::uint32_t account_id, SessionLocateCB cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	const auto opcode = util::enum_value(em::account::Opcode::CMSG_SESSION_LOOKUP);
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();

	auto builder = em::account::SessionLookupBuilder(*fbb);
	builder.add_account_id(account_id);
	fbb->Finish(builder.Finish());

	if(spark_.send(link_, opcode, fbb, [this, cb](auto link, auto message) {
		handle_locate_reply(link, message, cb);
	}) != spark::Service::Result::OK) {
		cb(em::account::Status::SERVER_LINK_ERROR, 0);
	}
}

void AccountService::locate_account_id(const std::string& username, IDLocateCB cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	const auto opcode = util::enum_value(em::account::Opcode::CMSG_ACCOUNT_LOOKUP);
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto fb_username = fbb->CreateString(username);

	auto builder = em::account::LookupIDBuilder(*fbb);
	builder.add_account_name(fb_username);
	fbb->Finish(builder.Finish());

	if(spark_.send(link_, opcode, fbb, [this, cb](auto link, auto message) {
		handle_id_locate_reply(link, message, cb);
	}) != spark::Service::Result::OK) {
		cb(em::account::Status::SERVER_LINK_ERROR, 0);
	}
}

} // ember