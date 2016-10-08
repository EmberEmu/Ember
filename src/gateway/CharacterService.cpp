/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CharacterService.h"
#include <spark/Helpers.h>
#include <shared/util/EnumHelper.h>
#include <boost/uuid/uuid.hpp>

namespace em = ember::messaging;

namespace ember {

CharacterService::CharacterService(spark::Service& spark, spark::ServiceDiscovery& s_disc,
                                   const Config& config, log::Logger* logger)
                                   : spark_(spark), s_disc_(s_disc), config_(config), logger_(logger) {
	spark_.dispatcher()->register_handler(this, em::Service::CHARACTER,
	                                      spark::EventDispatcher::Mode::CLIENT);
	listener_ = std::move(s_disc_.listener(messaging::Service::CHARACTER,
	                      std::bind(&CharacterService::service_located, this, std::placeholders::_1)));
	listener_->search();
}

CharacterService::~CharacterService() {
	spark_.dispatcher()->remove_handler(this);
}

void CharacterService::on_message(const spark::Link& link, const spark::Message& message) {
	// we only care about tracked messages at the moment
	LOG_WARN(logger_) << "Character service received unhandled message" << LOG_ASYNC;
}

void CharacterService::on_link_up(const spark::Link& link) {
	LOG_INFO(logger_) << "Link up: " << link.description << LOG_ASYNC;
	link_ = link;
}

void CharacterService::on_link_down(const spark::Link& link) {
	LOG_INFO(logger_) << "Link down: " << link.description << LOG_ASYNC;
}

void CharacterService::service_located(const messaging::multicast::LocateAnswer* message) {
	LOG_DEBUG(logger_) << "Located character service at " << message->ip()->str() 
	                   << ":" << message->port() << LOG_ASYNC;
	spark_.connect(message->ip()->str(), message->port());
}

void CharacterService::handle_reply(const spark::Link& link,
                                    boost::optional<const spark::Message> message,
                                    const ResponseCB& cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!message || !spark::Service::verify<em::character::CreateResponse>(*message)) {
		cb(em::character::Status::SERVER_LINK_ERROR, protocol::Result::RESPONSE_FAILURE);
		return;
	}

	auto data = flatbuffers::GetRoot<em::character::CreateResponse>(message->data);
	cb(data->status(), static_cast<protocol::Result>(data->result()));
}

void CharacterService::handle_rename_reply(const spark::Link& link,
                                           boost::optional<const spark::Message> message,
                                           const RenameCB& cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!message || !spark::Service::verify<em::character::RenameResponse>(*message)) {
		cb(em::character::Status::SERVER_LINK_ERROR, protocol::Result::CHAR_NAME_FAILURE, 0, "");
		return;
	}

	auto data = flatbuffers::GetRoot<em::character::RenameResponse>(message->data);

	// TODO! Refactor everything here - just pass the full character struct to the caller
	if(!data->name() || !data->character_id() || data->result() != (uint32_t)protocol::Result::RESPONSE_SUCCESS) {
		cb(em::character::Status::ILLFORMED_MESSAGE, protocol::Result::CHAR_NAME_FAILURE, 0, "");
		return;
	}

	cb(data->status(), static_cast<protocol::Result>(data->result()),
	   data->character_id(), data->name()->str());
}

void CharacterService::handle_retrieve_reply(const spark::Link& link,
                                             boost::optional<const spark::Message> message,
                                             const RetrieveCB& cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!message || !spark::Service::verify<em::character::RetrieveResponse>(*message)) {
		cb(em::character::Status::SERVER_LINK_ERROR, {});
		return;
	}

	std::vector<Character> characters;
	auto data = flatbuffers::GetRoot<em::character::RetrieveResponse>(message->data);
	auto key = data->characters();

	for(auto i = key->begin(); i != key->end(); ++i) {
		Character character;
		character.name = i->name()->str();
		character.id = i->id();
		character.account_id = i->account_id();
		character.realm_id = i->realm_id();
		character.race = i->race();
		character.class_ = i->class_();
		character.gender = i->gender();
		character.skin = i->skin();
		character.face = i->face();
		character.hairstyle = i->hairstyle();
		character.haircolour = i->haircolour();
		character.facialhair = i->facialhair();
		character.level = i->level();
		character.zone = i->zone();
		character.map = i->map();
		character.guild_id = i->guild_id();
		character.guild_rank = i->guild_rank();
		character.position.x = i->x();
		character.position.y = i->y();
		character.position.z = i->z();
		character.flags = static_cast<Character::Flags>(i->flags());
		character.first_login = i->first_login();
		character.pet_display = i->pet_display_id();
		character.pet_level = i->pet_level();
		character.pet_family = i->pet_family();
		characters.emplace_back(character);
	}

	cb(data->status(), characters);
}

void CharacterService::create_character(std::uint32_t account_id, const CharacterTemplate& character,
                                        ResponseCB cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	const auto opcode = util::enum_value(em::character::Opcode::CMSG_CHAR_CREATE);

	em::character::CharacterTemplateBuilder cbb(*fbb);
	cbb.add_name(fbb->CreateString(character.name));
	cbb.add_race(character.race);
	cbb.add_class_(character.class_);
	cbb.add_gender(character.gender);
	cbb.add_skin(character.skin);
	cbb.add_face(character.face);
	cbb.add_hairstyle(character.hairstyle);
	cbb.add_haircolour(character.haircolour);
	cbb.add_facialhair(character.facialhair);
	auto fb_char = cbb.Finish();

	messaging::character::CreateBuilder msg(*fbb);
	msg.add_account_id(account_id);
	msg.add_character(fb_char);
	msg.add_realm_id(config_.realm->id);
	msg.Finish();

	if(spark_.send(link_, opcode, fbb, [cb](auto link, auto token, auto data) {
		handle_reply(link, token, cb);
	}) != spark::Service::Result::OK) {
		cb(em::character::Status::SERVER_LINK_ERROR, {});
	}
}

void CharacterService::rename_character(std::uint32_t account_id, std::uint64_t character_id,
                                        const std::string& name, RenameCB cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;


	const auto opcode = util::enum_value(em::character::Opcode::CMSG_CHAR_RENAME);
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto fb_name = fbb->CreateString(name);

	em::character::RenameBuilder msg(*fbb);
	msg.add_account_id(account_id);
	msg.add_character_id(character_id);
	msg.add_realm_id(config_.realm->id);
	msg.add_name(fb_name);
	msg.Finish();

	if(spark_.send(link_, opcode, fbb, [cb](auto link, auto token, auto data) {
		handle_rename_reply(link, token, cb);
	}) != spark::Service::Result::OK) {
		cb(em::character::Status::SERVER_LINK_ERROR, protocol::Result::CHAR_NAME_FAILURE, 0, nullptr);
	}
}
void CharacterService::retrieve_characters(std::uint32_t account_id, RetrieveCB cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	const auto opcode = util::enum_value(em::character::Opcode::CMSG_CHAR_ENUM);
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();

	em::character::RetrieveBuilder msg(*fbb);
	msg.add_account_id(account_id);
	msg.add_realm_id(config_.realm->id);
	msg.Finish();

	if(spark_.send(link_, opcode, fbb, [cb](auto link, auto token, auto data) {
		handle_retrieve_reply(link, token, cb);
	}) != spark::Service::Result::OK) {
		std::vector<Character> chars;
		cb(em::character::Status::SERVER_LINK_ERROR, chars);
	}
}

void CharacterService::delete_character(std::uint32_t account_id, std::uint64_t id,
                                        ResponseCB cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	const auto opcode = util::enum_value(em::character::Opcode::CMSG_CHAR_DELETE);
	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();

	em::character::DeleteBuilder msg(*fbb);
	msg.add_account_id = account_id;
	msg.add_character_id = id;
	msg.add_realm_id = config_.realm->id;
	msg.Finish();

	if(spark_.send(link_, opcode, fbb, [cb](auto link, auto token, auto data) {
		handle_reply(link, token, cb);
	}) != spark::Service::Result::OK) {
		cb(em::character::Status::SERVER_LINK_ERROR, {});
	}
}

} // ember