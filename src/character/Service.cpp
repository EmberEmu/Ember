/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Service.h"

 /* TODO, TEMPORARY CODE FOR EXPERIMENTATION */

namespace em = ember::messaging;

namespace ember {

Service::Service(dal::CharacterDAO& character_dao, spark::Service& spark, spark::ServiceDiscovery& discovery,
                 log::Logger* logger)
                 : character_dao_(character_dao), spark_(spark), discovery_(discovery), logger_(logger) {
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

// todo, entirely temporary code, no validation, etc
void Service::retrieve_characters(const spark::Link& link, const em::MessageRoot* root) try {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto msg = static_cast<const em::character::Retrieve*>(root->data());
	auto ok = msg->account_name()->str();
	auto characters = character_dao_.characters(ok);
	send_character_list(link, root, std::move(characters));
} catch(std::exception& e) {
	LOG_WARN(logger_) << e.what() << LOG_ASYNC;
}


void Service::create_character(const spark::Link& link, const em::MessageRoot* root) try {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto msg = static_cast<const em::character::Create*>(root->data());
	auto c = msg->character();
	
	// temporary, if it isn't obvious
	Character character(
		c->name()->str(),
		0, // character ID, erp
		1, // account ID
		msg->realm_id(),
		c->race(),
		c->class_(),
		c->gender(),
		c->skin(),
		c->face(),
		c->hairstyle(),
		c->haircolour(),
		c->facialhair(),
		1, // level
		0, // zone
		0, // map,
		0, // guild ID
		0.f, 0.f, 0.f, // x y z
		0, // flags
		true, // first login
		0, // pet display
		0, // pet level
		0 // pet family
	);

	character_dao_.create(character);
	send_response(link, root, messaging::character::Status::OK);
} catch(std::exception& e) {
	LOG_WARN(logger_) << e.what() << LOG_ASYNC;
}

void Service::rename_character(const spark::Link& link, const em::MessageRoot* root) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto msg = static_cast<const em::character::Rename*>(root->data());
	// todo
	send_response(link, root, messaging::character::Status::OK);
}

void Service::send_character_list(const spark::Link& link, const em::MessageRoot* root,
                                  std::vector<Character> characters) {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto fbb = std::make_shared<flatbuffers::FlatBufferBuilder>();
	em::character::RetrieveResponseBuilder rrb(*fbb);

	// painful
	std::vector<flatbuffers::Offset<em::character::Character>> chars;

	for(auto character : characters) {
		em::character::CharacterBuilder cbb(*fbb);
		cbb.add_id(character.id());
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
		chars.push_back(cbb.Finish());
	}
	
	rrb.add_characters(fbb->CreateVector(chars));
	rrb.add_status(em::character::Status::OK);
	auto data_offset = rrb.Finish();
	em::MessageRootBuilder mrb(*fbb);
	mrb.add_service(em::Service::Character);
	mrb.add_data_type(em::Data::RetrieveResponse);
	mrb.add_data(data_offset.Union());

	if(root) {
		spark_.set_tracking_data(root, mrb, fbb.get());
	}

	auto mloc = mrb.Finish();
	fbb->Finish(mloc);
	spark_.send(link, fbb);
}

// todo
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

// todo
void Service::delete_character(const spark::Link& link, const em::MessageRoot* root) try {
	LOG_TRACE(logger_) << __func__ << LOG_ASYNC;

	auto msg = static_cast<const em::character::Delete*>(root->data());
	character_dao_.delete_character(msg->character_id());
	send_response(link, root, messaging::character::Status::OK);
} catch(std::exception& e) {
	LOG_WARN(logger_) << e.what() << LOG_ASYNC;
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