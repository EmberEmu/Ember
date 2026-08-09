/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CharacterClient.h"
#include <logger/Logger.h>
#include <shared/Realm.h>
#include <memory>

namespace ember::realm {

using namespace rpc::Character;

CharacterClient::CharacterClient(spark::Server& server, const ConfigStore& config_store, log::Logger& logger)
	: services::CharacterClient(server)
	, config_store_(config_store)
	, logger_(logger) {
	connect("127.0.0.1", 6001); // temp
}

void CharacterClient::on_link_up(const spark::Link& link) {
	LOG_DEBUG(logger_, "Link up: {}", link.peer_banner);
	link_ = link;
}

void CharacterClient::on_link_down(const spark::Link& link) {
	LOG_DEBUG(logger_, "Link down: {}", link.peer_banner);
}

void CharacterClient::connect_failed(const std::string_view ip, const std::uint16_t port) {
	LOG_INFO(logger_, "Failed to connect to character service on {}:{}", ip, port);
}

void CharacterClient::retrieve_characters(const std::uint32_t account_id, RetrieveCB cb) const {
	LOG_TRACE(logger_, log_func);

	const auto& config = config_store_.config();

	RetrieveT msg {
		.account_id = account_id,
		.realm_id = config.realm.id
	};

	send<RetrieveResponse>(msg, link_, [this, cb](auto link, auto message) {
		handle_retrieve_reply(link, message, cb);
	});
}

void CharacterClient::create_character(const std::uint32_t account_id,
									   const CharacterTemplateT& character,
									   ResponseCB cb) const {
	LOG_TRACE(logger_, log_func);

	const auto& config = config_store_.config();

	CreateT msg;
	msg.account_id = account_id;
	msg.realm_id = config.realm.id;
	msg.character = std::make_unique<CharacterTemplateT>(character);

	send<CreateResponse>(msg, link_, [this, cb](auto link, auto message) {
		handle_create_reply(link, message, cb);
	});
}

void CharacterClient::delete_character(std::uint32_t account_id,
									   std::uint64_t id,
									   ResponseCB cb) const {
	LOG_TRACE(logger_, log_func);

	const auto& config = config_store_.config();

	DeleteT msg {
		.account_id = account_id,
		.realm_id = config.realm.id,
		.character_id = id,
	};

	send<DeleteResponse>(msg, link_, [this, cb](auto link, auto message) {
		handle_delete_reply(link, message, cb);
	});
}

void CharacterClient::rename_character(std::uint32_t account_id,
									   std::uint64_t character_id,
									   const utf8_string& name,
									   RenameCB cb) const {
	LOG_TRACE(logger_, log_func);
	const auto& config = config_store_.config();

	RenameT msg {
		.account_id = account_id,
		.name = name,
		.realm_id = config.realm.id,
		.character_id = character_id,
	};

	send<RenameResponse>(msg, link_, [this, cb](auto link, auto message) {
		handle_rename_reply(link, message, cb);
	});
}

void CharacterClient::handle_create_reply(const spark::Link& link,
                                          std::expected<const CreateResponse*, spark::Result> res,
                                          ResponseCB cb) const {
	LOG_TRACE(logger_, log_func);

	if(!res) {
		cb(protocol::Result::char_create_error);
		return;
	}

	const auto msg = *res;
	cb(protocol::Result(msg->result()));
}

void CharacterClient::handle_rename_reply(const spark::Link& link,
                                          std::expected<const RenameResponse*, spark::Result> res,
                                          RenameCB cb) const {
	LOG_TRACE(logger_, log_func);

	if(!res) {
		cb(protocol::Result::char_name_failure, 0, "");
		return;
	}

	const auto msg = *res;

	if(!msg->name()) {
		cb(protocol::Result::char_name_failure, 0, "");
		return;
	}

	cb(protocol::Result(msg->result()), msg->character_id(), msg->name()->str());
}

void CharacterClient::handle_retrieve_reply(const spark::Link& link,
                                            std::expected<const RetrieveResponse*, spark::Result> res,
                                            RetrieveCB cb) const {
	LOG_TRACE(logger_, log_func);

	if(!res) {
		cb(Status::unknown_error, {});
		return;
	}

	const auto msg = *res;

	if(!msg->characters()) {
		cb(Status::ok, {});
		return;
	}

	std::vector<ember::Character> characters;
	characters.reserve(msg->characters()->size());

	for(const auto key : *msg->characters()) {
		ember::Character character;
		character.name = key->name()->str();
		character.id = key->id();
		character.account_id = key->account_id();
		character.realm_id = key->realm_id();
		character.race = key->race();
		character.class_ = key->class_();
		character.gender = key->gender();
		character.skin = key->skin();
		character.face = key->face();
		character.hairstyle = key->hairstyle();
		character.haircolour = key->haircolour();
		character.facialhair = key->facialhair();
		character.level = key->level();
		character.zone = key->zone();
		character.map = key->map();
		character.guild_id = key->guild_id();
		character.guild_rank = key->guild_rank();
		character.position.x = key->x();
		character.position.y = key->y();
		character.position.z = key->z();
		character.orientation = key->orientation();
		character.flags = static_cast<ember::Character::Flags>(key->flags());
		character.first_login = key->first_login();
		character.pet_display = key->pet_display_id();
		character.pet_level = key->pet_level();
		character.pet_family = key->pet_family();
		characters.emplace_back(std::move(character));
	}

	cb(Status::ok, std::move(characters));
}

void CharacterClient::handle_delete_reply(const spark::Link& link,
                                          std::expected<const DeleteResponse*, spark::Result> res,
                                          ResponseCB cb) const {
	LOG_TRACE(logger_, log_func);

	if(!res) {
		cb(protocol::Result::char_delete_failed);
		return;
	}

	const auto msg = *res;
	LOG_TRACE(logger_, "Result: {}", msg->result());
	cb(protocol::Result(msg->result()));
}

} // realm, ember