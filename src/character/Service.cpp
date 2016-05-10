/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Service.h"

namespace em = ember::messaging;

namespace ember {

Service::Service(spark::Service& spark, spark::ServiceDiscovery& discovery, log::Logger* logger)
                 : spark_(spark), discovery_(discovery), logger_(logger) {
	spark_.dispatcher()->register_handler(this, em::Service::Character, spark::EventDispatcher::Mode::SERVER);
	discovery_.register_service(em::Service::Character);
}

Service::~Service() {
	discovery_.remove_service(em::Service::Character);
	spark_.dispatcher()->remove_handler(this);
}

void Service::handle_message(const spark::Link& link, const em::MessageRoot* msg) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	switch(msg->data_type()) {
		case em::Data::Retrieve:
			retrieve_characters(link, msg);
			break;
		case em::Data::Create:
			create_character(link, msg);
			break;
		case em::Data::Rename:
			rename_character(link, msg);
			break;
		case em::Data::Delete:
			delete_character(link, msg);
			break;
		default:
			LOG_DEBUG(logger_) << "Service received unhandled message type" << LOG_ASYNC;
	}
}

void Service::retrieve_characters(const spark::Link& link, const em::MessageRoot* root) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto msg = static_cast<const em::character::Retrieve*>(root->data());

	send_character_list(link, root);
}

void Service::create_character(const spark::Link& link, const em::MessageRoot* root) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto msg = static_cast<const em::character::Create*>(root->data());
	// todo
	send_response(link, root, messaging::character::Status::OK);
}

void Service::rename_character(const spark::Link& link, const em::MessageRoot* root) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto msg = static_cast<const em::character::Rename*>(root->data());
	// todo
	send_response(link, root, messaging::character::Status::OK);
}

void Service::send_character_list(const spark::Link& link, const em::MessageRoot* root) {

}

void Service::send_response(const spark::Link& link, const em::MessageRoot* root, messaging::character::Status status) {
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	em::character::CharResponseBuilder rb(*fbb);
	rb.add_status(status);
	auto data_offset = rb.Finish();

	em::MessageRootBuilder mrb(*fbb);
	mrb.add_service(em::Service::Character);
	mrb.add_data_type(em::Data::CharResponse);
	mrb.add_data(data_offset.Union());
	spark_.set_tracking_data(root, mrb, fbb.get());
	auto mloc = mrb.Finish();

	fbb->Finish(mloc);
	spark_.send(link, fbb);
}

void Service::delete_character(const spark::Link& link, const em::MessageRoot* root) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto msg = static_cast<const em::character::Delete*>(root->data());
	// todo
	send_response(link, root, messaging::character::Status::OK);
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

} // ember