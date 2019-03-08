/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Service.h"
#include "CharacterHandler.h"
#include <shared/util/EnumHelper.h>
#include <string>
#include <cstdint>

namespace em = ember::messaging;

namespace ember {

Service::Service(dal::CharacterDAO& character_dao, const CharacterHandler& handler, spark::Service& spark,
                 spark::ServiceDiscovery& discovery, log::Logger* logger)
                 : character_dao_(character_dao), handler_(handler), spark_(spark),
                   discovery_(discovery), logger_(logger) {
	REGISTER(em::character::Opcode::CMSG_CHAR_CREATE, em::character::Create, Service::create_character);
	REGISTER(em::character::Opcode::CMSG_CHAR_DELETE, em::character::Delete, Service::delete_character);
	REGISTER(em::character::Opcode::CMSG_CHAR_RENAME, em::character::Rename, Service::rename_character);
	REGISTER(em::character::Opcode::CMSG_CHAR_ENUM, em::character::Retrieve, Service::retrieve_characters);

	spark_.dispatcher()->register_handler(this, em::Service::CHARACTER, spark::EventDispatcher::Mode::SERVER);
	discovery_.register_service(em::Service::CHARACTER);
}

Service::~Service() {
	discovery_.remove_service(em::Service::CHARACTER);
	spark_.dispatcher()->remove_handler(this);
}

void Service::on_link_up(const spark::Link& link) {
	LOG_INFO(logger_) << "Link up: " << link.description << LOG_ASYNC;
}

void Service::on_link_down(const spark::Link& link) {
	LOG_INFO(logger_) << "Link down: " << link.description << LOG_ASYNC;
}

void Service::on_message(const spark::Link& link, const spark::Message& message) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto handler = handlers_.find(static_cast<messaging::character::Opcode>(message.opcode));

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

void Service::retrieve_characters(const spark::Link& link, const spark::Message& message) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto msg = flatbuffers::GetRoot<em::character::Retrieve>(message.data);
	auto token = message.token;

	handler_.enumerate(msg->account_id(), msg->realm_id(), [this, link, token](const auto& chars) {
		if(chars) {
			send_character_list(link, token, em::character::Status::OK, *chars);
		} else {
			send_character_list(link, token, em::character::Status::UNKNOWN_ERROR, {});
		}
	});
}

void Service::create_character(const spark::Link& link, const spark::Message& message) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto token = message.token;
	auto msg = flatbuffers::GetRoot<em::character::Create>(message.data);

	if(!msg->character()) {
		LOG_WARN(logger_) << "Illformed create request from " << link.description << LOG_ASYNC;

		send_response(link, token, em::character::Status::ILLFORMED_MESSAGE,
		              protocol::Result::CHAR_LIST_FAILED);
		return;
	}

	handler_.create(msg->account_id(), msg->realm_id(), *msg->character(), [&, link, token](auto res) {
		LOG_DEBUG(logger_) << "Create response code: " << protocol::to_string(res) << LOG_ASYNC;
		send_response(link, token, messaging::character::Status::OK, res);
	});
}

void Service::send_character_list(const spark::Link& link, const spark::Beacon& token,
                                  em::character::Status status,
                                  const std::vector<Character>& characters) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto opcode = util::enum_value(em::character::Opcode::SMSG_CHAR_ENUM);
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();

	std::vector<flatbuffers::Offset<em::character::Character>> chars;

	for(auto character : characters) { // painful
		em::character::CharacterBuilder cbb(*fbb);
		cbb.add_id(character.id);
		cbb.add_name(fbb->CreateString(character.name));
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
	
	auto char_vec = fbb->CreateVector(chars);
	em::character::RetrieveResponseBuilder rrb(*fbb);
	rrb.add_status(status);
	rrb.add_characters(char_vec);
	fbb->Finish(rrb.Finish());

	spark_.send(link, opcode, fbb, token);
}

void Service::send_rename_response(const spark::Link& link, const spark::Beacon& token,
                                   protocol::Result result, const std::optional<Character>& character) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto opcode = util::enum_value(em::character::Opcode::SMSG_CHAR_RENAME);
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	em::character::RenameResponseBuilder rb(*fbb);
	rb.add_status(messaging::character::Status::OK);
	rb.add_result(static_cast<std::uint32_t>(result));

	if(result == protocol::Result::RESPONSE_SUCCESS) {
		rb.add_character_id(character->id);
		rb.add_name(fbb->CreateString(character->name));
	}

	rb.add_result(static_cast<std::uint32_t>(result));
	fbb->Finish(rb.Finish());

	spark_.send(link, opcode, fbb, token);
}

void Service::send_response(const spark::Link& link, const spark::Beacon& token,
                            em::character::Status status, protocol::Result result) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto opcode = util::enum_value(em::character::Opcode::SMSG_CHAR_RESPONSE);
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	em::character::CreateResponseBuilder rb(*fbb);

	rb.add_status(status);
	rb.add_result(static_cast<std::uint32_t>(result));
	fbb->Finish(rb.Finish());

	spark_.send(link, opcode, fbb, token);
}

void Service::delete_character(const spark::Link& link, const spark::Message& message) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto token = message.token;
	auto msg = flatbuffers::GetRoot<em::character::Delete>(message.data);

	handler_.erase(msg->account_id(), msg->realm_id(), msg->character_id(), [&, link, token](auto res) {
		LOG_DEBUG(logger_) << "Deletion response code: " << protocol::to_string(res) << LOG_ASYNC;
		send_response(link, message.token, messaging::character::Status::OK, res);
	});
}

void Service::rename_character(const spark::Link& link, const spark::Message& message) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto msg = flatbuffers::GetRoot<em::character::Rename>(message.data);
	auto token = message.token;

	if(!msg->name() || !msg->account_id() || !msg->character_id()) {
		LOG_WARN(logger_) << "Illformed rename request from " << link.description << LOG_ASYNC;

		send_response(link, token, messaging::character::Status::ILLFORMED_MESSAGE,
		              protocol::Result::CHAR_NAME_FAILURE);
		return;
	}

	handler_.rename(msg->account_id(), msg->character_id(), msg->name()->str(),
	               [&, link, token](auto res, auto character) {
		LOG_DEBUG(logger_) << "Rename response code: " << protocol::to_string(res) << LOG_ASYNC;
		send_rename_response(link, token, res, std::move(character));
	});
}

} // ember