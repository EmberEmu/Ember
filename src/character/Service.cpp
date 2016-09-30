/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Service.h"
#include <game_protocol/ResultCodes.h>

 /* TODO, TEMPORARY CODE FOR EXPERIMENTATION */

namespace em = ember::messaging;

namespace ember {

Service::Service(dal::CharacterDAO& character_dao, const CharacterHandler& handler, spark::Service& spark,
                 spark::ServiceDiscovery& discovery, log::Logger* logger)
                 : character_dao_(character_dao), handler_(handler), spark_(spark),
                   discovery_(discovery), logger_(logger) {
	spark_.dispatcher()->register_handler(this, em::Service::Character, spark::EventDispatcher::Mode::SERVER);
	discovery_.register_service(em::Service::Character);
}

Service::~Service() {
	discovery_.remove_service(em::Service::Character);
	spark_.dispatcher()->remove_handler(this);
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
	std::vector<std::uint8_t> tracking(root->tracking_id()->begin(), root->tracking_id()->end());

	handler_.enumerate(msg->account_id(), msg->realm_id(), [&, tracking](const auto& chars) {
		send_character_list(link, tracking, chars);
	});
}

void Service::create_character(const spark::Link& link, const em::MessageRoot* root) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto msg = static_cast<const em::character::Create*>(root->data());
	std::vector<std::uint8_t> tracking(root->tracking_id()->begin(), root->tracking_id()->end());

	if(msg->character() == nullptr) {
		LOG_WARN(logger_) << "Illformed character create request from " << link.description << LOG_ASYNC;

		send_response(link, tracking, messaging::character::Status::ILLFORMED_MESSAGE,
		              protocol::Result::CHAR_LIST_FAILED);
		return;
	}

	handler_.create(msg->account_id(), msg->realm_id(), *msg->character(), [&, link, tracking](auto res) {
		LOG_DEBUG(logger_) << "Create response code: " << protocol::to_string(res) << LOG_ASYNC;
		send_response(link, tracking, messaging::character::Status::OK, res);
	});
}

void Service::send_character_list(const spark::Link& link, const std::vector<std::uint8_t>& tracking,
                                  const boost::optional<std::vector<Character>>& characters) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto id = fbb->CreateVector(tracking);

	// painful
	std::vector<flatbuffers::Offset<em::character::Character>> chars;

	if(characters) {
		for(auto character : *characters) {
			auto name = fbb->CreateString(character.name);

			em::character::CharacterBuilder cbb(*fbb);
			cbb.add_id(character.id);
			cbb.add_name(name);
			cbb.add_race(character.race);
			cbb.add_class_(character.class_);
			cbb.add_gender(character.gender);
			cbb.add_skin(character.skin);
			cbb.add_face(character.face);
			cbb.add_hairstyle(character.hairstyle);
			cbb.add_haircolour(character.haircolour);
			cbb.add_facialhair(character.facialhair);
			cbb.add_level(character.level);
			cbb.add_zone(character.zone);
			cbb.add_map(character.map);
			cbb.add_guild_id(character.guild_id);
			cbb.add_x(character.position.x);
			cbb.add_y(character.position.y);
			cbb.add_z(character.position.z);
			cbb.add_flags(static_cast<std::uint32_t>(character.flags));
			cbb.add_first_login(character.first_login);
			cbb.add_pet_display_id(character.pet_display);
			cbb.add_pet_level(character.pet_level);
			cbb.add_pet_family(character.pet_family);
			chars.push_back(cbb.Finish());
		}
	}

	auto charsvec = fbb->CreateVector(chars);

	em::character::RetrieveResponseBuilder rrb(*fbb);
	// todo, something weird going on with Flatbuffers - won't set correct status if done above
	if(characters) {
		rrb.add_status(em::character::Status::OK);
	} else {
		rrb.add_status(em::character::Status::UNKNOWN_ERROR);
	}
	
	rrb.add_characters(charsvec);
	auto data_offset = rrb.Finish();

	em::MessageRootBuilder mrb(*fbb);
	mrb.add_service(em::Service::Character);
	mrb.add_data_type(em::Data::RetrieveResponse);
	mrb.add_data(data_offset.Union());

	mrb.add_tracking_id(id);
	mrb.add_tracking_ttl(1);

	auto mloc = mrb.Finish();
	fbb->Finish(mloc);
	spark_.send(link, fbb);
}

void Service::send_rename_response(const spark::Link& link, const std::vector<std::uint8_t>& tracking,
								   messaging::character::Status status, protocol::Result result,
								   boost::optional<Character> character) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto id = fbb->CreateVector(tracking);
	auto name = fbb->CreateString(character->name);

	em::character::RenameResponseBuilder rb(*fbb);
	rb.add_status(status);
	rb.add_result(static_cast<std::uint32_t>(result));

	if(result == protocol::Result::RESPONSE_SUCCESS) {
		rb.add_character_id(character->id);
		rb.add_name(name);
	}

	rb.add_result(static_cast<std::uint32_t>(result));
	auto data_offset = rb.Finish();

	em::MessageRootBuilder mrb(*fbb);
	mrb.add_service(em::Service::Character);
	mrb.add_data_type(em::Data::RenameResponse);
	mrb.add_data(data_offset.Union());

	mrb.add_tracking_id(id);
	mrb.add_tracking_ttl(1);

	auto mloc = mrb.Finish();

	fbb->Finish(mloc);
	spark_.send(link, fbb);
}

void Service::send_response(const spark::Link& link, const std::vector<std::uint8_t>& tracking,
							messaging::character::Status status, protocol::Result result) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto id = fbb->CreateVector(tracking);

	em::character::CharResponseBuilder rb(*fbb);
	rb.add_status(status);
	rb.add_result(static_cast<std::uint32_t>(result));
	auto data_offset = rb.Finish();

	em::MessageRootBuilder mrb(*fbb);
	mrb.add_service(em::Service::Character);
	mrb.add_data_type(em::Data::CharResponse);
	mrb.add_data(data_offset.Union());

	mrb.add_tracking_id(id);
	mrb.add_tracking_ttl(1);

	auto mloc = mrb.Finish();

	fbb->Finish(mloc);
	spark_.send(link, fbb);
}

void Service::delete_character(const spark::Link& link, const em::MessageRoot* root) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto msg = static_cast<const em::character::Delete*>(root->data());
	std::vector<std::uint8_t> tracking(root->tracking_id()->begin(), root->tracking_id()->end());

	handler_.erase(msg->account_id(), msg->realm_id(), msg->character_id(), [&, link, tracking](auto res) {
		LOG_DEBUG(logger_) << "Deletion response code: " << protocol::to_string(res) << LOG_ASYNC;
		send_response(link, tracking, messaging::character::Status::OK, res);
	});
}

void Service::rename_character(const spark::Link& link, const em::MessageRoot* root) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto msg = static_cast<const em::character::Rename*>(root->data());
	std::vector<std::uint8_t> tracking(root->tracking_id()->begin(), root->tracking_id()->end());

	if(!msg->name() || !msg->account_id() || !msg->character_id()) {
		LOG_WARN(logger_) << "Illformed rename request from " << link.description << LOG_ASYNC;

		send_response(link, tracking, messaging::character::Status::ILLFORMED_MESSAGE,
		              protocol::Result::CHAR_NAME_FAILURE);
		return;
	}

	handler_.rename(msg->account_id(), msg->character_id(), msg->name()->str(),
	               [&, link, tracking](auto res, boost::optional<Character> character) {
		LOG_DEBUG(logger_) << "Rename response code: " << protocol::to_string(res) << LOG_ASYNC;

		send_rename_response(link, tracking, messaging::character::Status::OK, res, character);
	});
}

} // ember