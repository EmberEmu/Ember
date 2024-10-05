/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CharacterClient.h"
#include <memory>

namespace ember {

CharacterClient::CharacterClient(spark::v2::Server& server, Config& config, log::Logger& logger)
	: services::Characterv2Client(server),
	  config_(config),
	  logger_(logger) {
	connect("127.0.0.1", 8003); // temp
}

void CharacterClient::on_link_up(const spark::v2::Link& link) {
	LOG_DEBUG_ASYNC(logger_, "Link up: {}", link.peer_banner);
	link_ = link;
}

void CharacterClient::on_link_down(const spark::v2::Link& link) {
	LOG_DEBUG_ASYNC(logger_, "Link up: {}", link.peer_banner);
}

void CharacterClient::retrieve_characters(const std::uint32_t account_id, RetrieveCB cb) const {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	em::RetrieveT msg {
		.account_id = account_id,
		.realm_id = config_.realm->id
	};

	send<em::RetrieveResponse>(msg, link_,
		[this, cb](auto link, auto message) {
			handle_retrieve_reply(link, message, cb);
		}
	);
}

void CharacterClient::create_character(const std::uint32_t account_id,
									   const CharacterTemplate& character,
									   ResponseCB cb) const {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	em::CharacterTemplateT tmpl {
		.name = character.name,
		.race = character.race,
		.class_ = character.class_,
		.gender = character.gender,
		.skin = character.skin,
		.face = character.face,
		.hairstyle = character.hairstyle,
		.haircolour = character.haircolour,
		.facialhair = character.facialhair
	};

	em::CreateT msg;
	msg.account_id = account_id;
	msg.realm_id = config_.realm->id;
	msg.character = std::make_unique<em::CharacterTemplateT>(std::move(tmpl));

	send<em::CreateResponse>(msg, link_,
		[this, cb](auto link, auto message) {
			handle_create_reply(link, message, cb);
		}
	);
}

void CharacterClient::delete_character(std::uint32_t account_id,
									   std::uint64_t id,
									   ResponseCB cb) const {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	em::DeleteT msg {
		.account_id = account_id,
		.realm_id = config_.realm->id,
		.character_id = id,
	};

	send<em::DeleteResponse>(msg, link_,
		[this, cb](auto link, auto message) {
			handle_delete_reply(link, message, cb);
		}
	);
}

void CharacterClient::rename_character(std::uint32_t account_id,
									   std::uint64_t character_id,
									   const utf8_string& name,
									   RenameCB cb) const {
	LOG_TRACE(logger_) << log_func << LOG_ASYNC;

	em::RenameT msg {
		.account_id = account_id,
		.name = name,
		.realm_id = config_.realm->id,
		.character_id = character_id,
	};

	send<em::RenameResponse>(msg, link_,
		[this, cb](auto link, auto message) {
			handle_rename_reply(link, message, cb);
		}
	);
}

void CharacterClient::handle_create_reply(
	const spark::v2::Link& link,
	std::expected<const em::CreateResponse*, spark::v2::Result> resp,
	ResponseCB cb) const {
	if(!resp) {
		cb(em::Status::UNKNOWN_ERROR, {});
		return;
	}

	const auto* msg = *resp;
	cb(msg->status(), protocol::Result(msg->result()));
}

void CharacterClient::handle_rename_reply(
	const spark::v2::Link& link,
	std::expected<const em::RenameResponse*, spark::v2::Result> resp,
	RenameCB cb) const {
	if(!resp) {
		cb(em::Status::UNKNOWN_ERROR, {}, 0, "");
		return;
	}

	const auto* msg = *resp;

	if(!msg->name()) {
		cb(em::Status::ILLFORMED_MESSAGE, {}, 0, "");
		return;
	}

	cb(msg->status(), protocol::Result(msg->result()), msg->character_id(), msg->name()->str());
}

void CharacterClient::handle_retrieve_reply(
	const spark::v2::Link& link,
	std::expected<const em::RetrieveResponse*, spark::v2::Result> resp,
	RetrieveCB cb) const {
	if(!resp) {
		cb(em::Status::UNKNOWN_ERROR, {});
		return;
	}

	const auto msg = *resp;

	if(!msg->characters()) {
		cb(em::Status::ILLFORMED_MESSAGE, {});
		return;
	}

	std::vector<Character> characters;
	auto key = msg->characters();

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
		character.orientation = i->orientation();
		character.flags = static_cast<Character::Flags>(i->flags());
		character.first_login = i->first_login();
		character.pet_display = i->pet_display_id();
		character.pet_level = i->pet_level();
		character.pet_family = i->pet_family();
		characters.emplace_back(std::move(character));
	}

	cb(msg->status(), std::move(characters));
}

void CharacterClient::handle_delete_reply(
	const spark::v2::Link& link,
	std::expected<const em::DeleteResponse*, spark::v2::Result> resp,
	ResponseCB cb) const {
	if(!resp) {
		cb(em::Status::UNKNOWN_ERROR, {});
		return;
	}

	const auto* msg = *resp;
	cb(msg->status(), protocol::Result(msg->result()));
}

} // ember