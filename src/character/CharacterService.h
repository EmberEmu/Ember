/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <CharacterServiceStub.h>
#include <protocol/ResultCodes.h>
#include <shared/database/objects/Character.h>
#include <span>

namespace ember {

class CharacterHandler;

class CharacterService final : public services::CharacterService {
	const CharacterHandler& handler_;
	log::Logger& logger_;

	void send_rename(const protocol::Result& res,
	                 std::optional<ember::Character>& character,
	                 const spark::v2::Link& link,
	                 const spark::v2::Token& token) const;

	void send_characters(const protocol::Result& res,
	                     std::span<ember::Character> characters,
	                     const spark::v2::Link& link,
	                     const spark::v2::Token& token) const;

	std::optional<rpc::Character::CreateResponseT> handle_create(
		const rpc::Character::Create& msg,
		const spark::v2::Link& link,
		const spark::v2::Token& token) override;

	std::optional<rpc::Character::DeleteResponseT> handle_delete(
		const rpc::Character::Delete& msg,
		const spark::v2::Link& link,
		const spark::v2::Token& token) override;

	std::optional<rpc::Character::RenameResponseT> handle_rename(
		const rpc::Character::Rename& msg,
		const spark::v2::Link& link,
		const spark::v2::Token& token) override;

	std::optional<rpc::Character::RetrieveResponseT> handle_enumerate(
		const rpc::Character::Retrieve& msg,
		const spark::v2::Link& link,
		const spark::v2::Token& token) override;

	void on_link_up(const spark::v2::Link& link) override;
	void on_link_down(const spark::v2::Link& link) override;

public:
	CharacterService(spark::v2::Server& spark, const CharacterHandler& handler, log::Logger& logger);
};

} // ember