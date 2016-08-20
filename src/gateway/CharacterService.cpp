/*
 * Copyright (c) 2015-2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CharacterService.h"
#include <boost/uuid/uuid.hpp>

namespace em = ember::messaging;

namespace ember {

CharacterService::CharacterService(spark::Service& spark, spark::ServiceDiscovery& s_disc, const Config& config, log::Logger* logger)
                                   : spark_(spark), s_disc_(s_disc), config_(config), logger_(logger) {
	spark_.dispatcher()->register_handler(this, em::Service::Character, spark::EventDispatcher::Mode::CLIENT);
	listener_ = std::move(s_disc_.listener(messaging::Service::Character,
	                      std::bind(&CharacterService::service_located, this, std::placeholders::_1)));
	listener_->search();
}

CharacterService::~CharacterService() {
	spark_.dispatcher()->remove_handler(this);
}

void CharacterService::handle_message(const spark::Link& link, const em::MessageRoot* root) {
	// we only care about tracked messages at the moment
	LOG_DEBUG(logger_) << "Session service received unhandled message" << LOG_ASYNC;
}

void CharacterService::handle_link_event(const spark::Link& link, spark::LinkState event) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	switch(event) {
		case spark::LinkState::LINK_UP:
			LOG_INFO(logger_) << "Link to character server established" << LOG_ASYNC;
			link_ = link;
			break;
		case spark::LinkState::LINK_DOWN:
			LOG_INFO(logger_) << "Link to character server closed" << LOG_ASYNC;
			break;
	}
}

void CharacterService::service_located(const messaging::multicast::LocateAnswer* message) {
	LOG_DEBUG(logger_) << "Located account service at " << message->ip()->str() 
	                   << ":" << message->port() << LOG_ASYNC;
	spark_.connect(message->ip()->str(), message->port());
}

void CharacterService::handle_reply(const spark::Link& link, const boost::uuids::uuid& uuid,
                                    boost::optional<const em::MessageRoot*> root, const ResponseCB& cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!root || (*root)->data_type() != messaging::Data::CharResponse) {
		cb(em::character::Status::SERVER_LINK_ERROR, {});
		return;
	}

	auto message = static_cast<const em::character::CharResponse*>((*root)->data());
	cb(message->status(), static_cast<protocol::ResultCode>(message->result()));
}

void CharacterService::handle_rename_reply(const spark::Link& link, const boost::uuids::uuid& uuid,
                                           boost::optional<const messaging::MessageRoot*> root,
                                           const RenameCB& cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	if(!root || (*root)->data_type() != messaging::Data::RenameResponse) {
		cb(em::character::Status::SERVER_LINK_ERROR, protocol::ResultCode::CHAR_NAME_FAILURE, 0, "");
		return;
	}

	auto message = static_cast<const messaging::character::RenameResponse*>((*root)->data());

	// TODO! Refactor everything here - just pass the full character struct to the caller
	if(!message->name() || !message->character_id() || message->result() != (uint32_t)protocol::ResultCode::RESPONSE_SUCCESS) {
		cb(em::character::Status::ILLFORMED_MESSAGE, protocol::ResultCode::CHAR_NAME_FAILURE, 0, "");
		return;
	}

	cb(message->status(), static_cast<protocol::ResultCode>(message->result()),
	   message->character_id(), message->name()->str());
}

void CharacterService::handle_retrieve_reply(const spark::Link& link, const boost::uuids::uuid& uuid,
                                             boost::optional<const messaging::MessageRoot*> root,
                                             const RetrieveCB& cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	std::vector<Character> characters;

	if(!root || (*root)->data_type() != messaging::Data::RetrieveResponse) {
		cb(em::character::Status::SERVER_LINK_ERROR, characters);
		return;
	}

	auto message = static_cast<const messaging::character::RetrieveResponse*>((*root)->data());
	auto key = message->characters();

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

	cb(message->status(), characters);
}

void CharacterService::create_character(std::uint32_t account_id, const CharacterTemplate& character,
                                        ResponseCB cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	
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

	auto uuid = generate_uuid();
	auto uuid_bytes = fbb->CreateVector(uuid.begin(), uuid.static_size());
	auto msg = messaging::CreateMessageRoot(*fbb, messaging::Service::Character, uuid_bytes, 0,
		em::Data::Create, em::character::CreateCreate(*fbb, account_id, config_.realm->id, fb_char).Union());
	fbb->Finish(msg);

	auto track_cb = std::bind(&CharacterService::handle_reply, this, std::placeholders::_1,
							  std::placeholders::_2, std::placeholders::_3, cb);

	if(spark_.send_tracked(link_, uuid, fbb, track_cb) != spark::Service::Result::OK) {
		cb(em::character::Status::SERVER_LINK_ERROR, {});
	}
}

void CharacterService::rename_character(std::uint32_t account_id, std::uint64_t character_id,
                                        const std::string& name, RenameCB cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();

	auto uuid = generate_uuid();
	auto uuid_bytes = fbb->CreateVector(uuid.begin(), uuid.static_size());
	auto msg = messaging::CreateMessageRoot(*fbb, messaging::Service::Character, uuid_bytes, 0,
	                                        em::Data::Rename, em::character::CreateRename(*fbb, account_id,
	                                        fbb->CreateString(name), config_.realm->id, character_id).Union());
	fbb->Finish(msg);

	auto track_cb = std::bind(&CharacterService::handle_rename_reply, this, std::placeholders::_1,
	                          std::placeholders::_2, std::placeholders::_3, cb);

	if(spark_.send_tracked(link_, uuid, fbb, track_cb) != spark::Service::Result::OK) {
		cb(em::character::Status::SERVER_LINK_ERROR, protocol::ResultCode::CHAR_NAME_FAILURE, 0, nullptr);
	}
}
void CharacterService::retrieve_characters(std::uint32_t account_id, RetrieveCB cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto uuid = generate_uuid();
	auto uuid_bytes = fbb->CreateVector(uuid.begin(), uuid.static_size());
	auto msg = messaging::CreateMessageRoot(*fbb, messaging::Service::Character, uuid_bytes, 0,
		em::Data::Retrieve, em::character::CreateRetrieve(*fbb, account_id, config_.realm->id).Union());
	fbb->Finish(msg);

	auto track_cb = std::bind(&CharacterService::handle_retrieve_reply, this, std::placeholders::_1,
	                          std::placeholders::_2, std::placeholders::_3, cb);

	if(spark_.send_tracked(link_, uuid, fbb, track_cb) != spark::Service::Result::OK) {
		std::vector<Character> chars;
		cb(em::character::Status::SERVER_LINK_ERROR, chars);
	}
}

void CharacterService::delete_character(std::uint32_t account_id, std::uint64_t id, ResponseCB cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto uuid = generate_uuid();
	auto uuid_bytes = fbb->CreateVector(uuid.begin(), uuid.static_size());
	auto msg = messaging::CreateMessageRoot(*fbb, messaging::Service::Character, uuid_bytes, 0,
		em::Data::Delete, em::character::CreateDelete(*fbb, account_id, config_.realm->id, id).Union());
	fbb->Finish(msg);

	auto track_cb = std::bind(&CharacterService::handle_reply, this, std::placeholders::_1,
	                          std::placeholders::_2, std::placeholders::_3, cb);

	if(spark_.send_tracked(link_, uuid, fbb, track_cb) != spark::Service::Result::OK) {
		cb(em::character::Status::SERVER_LINK_ERROR, {});
	}
}

} // ember