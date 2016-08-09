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

CharacterService::CharacterService(spark::Service& spark, spark::ServiceDiscovery& s_disc, log::Logger* logger)
                               : spark_(spark), s_disc_(s_disc), logger_(logger) {
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
		cb(em::character::Status::SERVER_LINK_ERROR);
		return;
	}

	auto message = static_cast<const em::character::CharResponse*>((*root)->data());
	cb(message->status());
}

void CharacterService::handle_retrieve_reply(const spark::Link& link, const boost::uuids::uuid& uuid,
                                             boost::optional<const messaging::MessageRoot*> root, const RetrieveCB& cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	std::vector<Character> characters;

	if(!root || (*root)->data_type() != messaging::Data::RetrieveResponse) {
		cb(em::character::Status::SERVER_LINK_ERROR, characters);
		return;
	}

	auto message = static_cast<const messaging::character::RetrieveResponse*>((*root)->data());
	auto key = message->characters();

	for(auto i = key->begin(); i != key->end(); ++i) {
		Character character (
			i->name()->str(),
			i->id(),
			0, // account ID
			0, // realm ID
			i->race(),
			i->class_(),
			i->gender(),
			i->skin(),
			i->face(),
			i->hairstyle(),
			i->haircolour(),
			i->facialhair(),
			i->level(), // level
			i->zone(), // zone
			i->map(), // map,
			i->guild_id(), // guild ID
			i->x(),
			i->y(),
			i->z(),
			i->flags(),
			i->first_login(),
			i->pet_display_id(),
			i->pet_level(),
			i->pet_family()
		);

		characters.emplace_back(character);
	}

	cb(message->status(), characters);
}

void CharacterService::create_character(std::string account_name, const Character& character, ResponseCB cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	
	em::character::CharacterBuilder cbb(*fbb);
	cbb.add_id(0); // ??
	cbb.add_name(fbb->CreateString(character.name()));
	cbb.add_race(character.race());
	cbb.add_class_(character.class_temp());
	cbb.add_gender(character.gender());
	cbb.add_skin(character.skin());
	cbb.add_face(character.face());
	cbb.add_hairstyle(character.hairstyle());
	cbb.add_haircolour(character.haircolour());
	cbb.add_facialhair(character.facialhair());
	cbb.add_level(character.level());
	cbb.add_zone(character.zone());
	cbb.add_map(character.map());
	cbb.add_guild_id(character.guild_id());
	cbb.add_x(character.x());
	cbb.add_y(character.y());
	cbb.add_z(character.z());
	cbb.add_flags(character.flags());
	cbb.add_first_login(character.first_login());
	cbb.add_pet_display_id(character.pet_display());
	cbb.add_pet_level(character.pet_level());
	cbb.add_pet_family(character.pet_family());
	auto fb_char = cbb.Finish();

	auto uuid = generate_uuid();
	auto uuid_bytes = fbb->CreateVector(uuid.begin(), uuid.static_size());
	auto msg = messaging::CreateMessageRoot(*fbb, messaging::Service::Character, uuid_bytes, 0,
		em::Data::Create, em::character::CreateCreate(*fbb, 1, fb_char).Union());
	fbb->Finish(msg);

	auto track_cb = std::bind(&CharacterService::handle_reply, this, std::placeholders::_1,
							  std::placeholders::_2, std::placeholders::_3, cb);

	if(spark_.send_tracked(link_, uuid, fbb, track_cb) != spark::Service::Result::OK) {
		cb(em::character::Status::SERVER_LINK_ERROR);
	}
}

void CharacterService::retrieve_characters(std::string account_name, RetrieveCB cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto uuid = generate_uuid();
	auto uuid_bytes = fbb->CreateVector(uuid.begin(), uuid.static_size());
	auto msg = messaging::CreateMessageRoot(*fbb, messaging::Service::Character, uuid_bytes, 0,
		em::Data::Retrieve, em::account::CreateKeyLookup(*fbb, fbb->CreateString(account_name)).Union());
	fbb->Finish(msg);

	auto track_cb = std::bind(&CharacterService::handle_retrieve_reply, this, std::placeholders::_1,
							  std::placeholders::_2, std::placeholders::_3, cb);

	if(spark_.send_tracked(link_, uuid, fbb, track_cb) != spark::Service::Result::OK) {
		std::vector<Character> chars;
		cb(em::character::Status::SERVER_LINK_ERROR, chars);
	}
}

void CharacterService::delete_character(std::string account_name, std::uint64_t id, ResponseCB cb) const {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	auto uuid = generate_uuid();
	auto uuid_bytes = fbb->CreateVector(uuid.begin(), uuid.static_size());
	auto msg = messaging::CreateMessageRoot(*fbb, messaging::Service::Character, uuid_bytes, 0,
		em::Data::Delete, em::character::CreateDelete(*fbb, fbb->CreateString(account_name), 1, id).Union());
	fbb->Finish(msg);

	auto track_cb = std::bind(&CharacterService::handle_reply, this, std::placeholders::_1,
	                          std::placeholders::_2, std::placeholders::_3, cb);

	if(spark_.send_tracked(link_, uuid, fbb, track_cb) != spark::Service::Result::OK) {
		cb(em::character::Status::SERVER_LINK_ERROR);
	}
}

} // ember