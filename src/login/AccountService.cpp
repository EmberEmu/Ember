/*
 * Copyright (c) 2015 - 2022 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "AccountService.h"
#include <boost/uuid/uuid.hpp>
#include <utility>

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

void AccountService::on_message(const spark::Link& link, const spark::Message& message) {
	// we only care about tracked messages at the moment
	LOG_DEBUG(logger_) << "Session service received unhandled message" << LOG_ASYNC;
}

void AccountService::on_link_up(const spark::Link& link) {
	LOG_INFO(logger_) << "Link up: " << link.description << LOG_ASYNC;
	link_ = link;
}

void AccountService::on_link_down(const spark::Link& link) {
	LOG_INFO(logger_) << "Link down: " << link.description << LOG_ASYNC;
}

void AccountService::service_located(const messaging::multicast::LocateResponse* message) {
	LOG_DEBUG(logger_) << "Located account service at " << message->ip()->str() 
	                   << ":" << message->port() << LOG_ASYNC;
	spark_.connect(message->ip()->str(), message->port());
}

void AccountService::handle_register_reply(const spark::Link& link,
                                           std::optional<spark::Message>& message,
                                           const RegisterCB& cb) const {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	// todo, validate
	if(!message) {
		cb(em::account::Status::SERVER_LINK_ERROR);
		return;
	}

	auto response = flatbuffers::GetRoot<em::account::Response>(message->data);
	cb(response->status());
}

void AccountService::handle_locate_reply(const spark::Link& link,
                                         std::optional<spark::Message>& message,
                                         const LocateCB& cb) const {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	// todo, validate
	if(!message) {
		cb(em::account::Status::SERVER_LINK_ERROR, 0);
		return;
	}

	auto response = flatbuffers::GetRoot<em::account::SessionResponse>(message->data);
	auto key = response->key();

	if(!key) {
		cb(response->status(), 0);
		return;
	}

	cb(response->status(), Botan::BigInt::decode(key->data(), key->size()));
}

void AccountService::locate_session(std::uint32_t account_id, LocateCB cb) const {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	constexpr auto opcode = std::to_underlying(em::account::Opcode::CMSG_ACCOUNT_LOOKUP);
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

void AccountService::register_session(std::uint32_t account_id, const srp6::SessionKey& key,
                                      RegisterCB cb) const {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	constexpr auto opcode = std::to_underlying(messaging::account::Opcode::CMSG_REGISTER_SESSION);
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto f_key = fbb->CreateVector(key.t.data(), key.t.size());

	auto builder = messaging::account::RegisterSessionBuilder(*fbb);
	builder.add_account_id(account_id);
	builder.add_key(f_key);
	fbb->Finish(builder.Finish());
	
	if(spark_.send(link_, opcode, fbb, [this, cb](auto link, auto message) {
		handle_register_reply(link, message, cb);
	}) != spark::Service::Result::OK) {
		cb(em::account::Status::SERVER_LINK_ERROR);
	}
}

} // ember