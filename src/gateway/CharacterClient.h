/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Config.h"
#include <logger/Logger.h>
#include <Characterv2ClientStub.h>
#include <protocol/ResultCodes.h>
#include <shared/database/objects/Character.h>

namespace ember {

class CharacterClient final : public services::Characterv2Client {
public:
	using ResponseCB = std::function<void(messaging::Characterv2::Status, protocol::Result)>;
	using RenameCB = std::function<void(messaging::Characterv2::Status, protocol::Result, std::uint64_t, std::string)>;
	using RetrieveCB = std::function<void(messaging::Characterv2::Status, std::vector<Character>)>;

private:
	const Config& config_;
	spark::v2::Link link_;
	log::Logger& logger_;

	void on_link_up(const spark::v2::Link& link) override;
	void on_link_down(const spark::v2::Link& link) override;

	void handle_create_reply(
		const spark::v2::Link& link,
		std::expected<const messaging::Characterv2::CreateResponse*, spark::v2::Result> resp,
		ResponseCB cb) const;

	void handle_rename_reply(
		const spark::v2::Link& link,
		std::expected<const messaging::Characterv2::RenameResponse*, spark::v2::Result> resp,
		RenameCB cb) const;

	void handle_retrieve_reply(
		const spark::v2::Link& link,
		std::expected<const messaging::Characterv2::RetrieveResponse*, spark::v2::Result> resp,
		RetrieveCB cb) const;

	void handle_delete_reply(
		const spark::v2::Link& link,
		std::expected<const messaging::Characterv2::DeleteResponse*, spark::v2::Result> resp,
		ResponseCB cb) const;

public:
	CharacterClient(spark::v2::Server& server, Config& config, log::Logger& logger);

	void retrieve_characters(std::uint32_t account_id,
	                         RetrieveCB cb) const;

	void create_character(std::uint32_t account_id,
	                      const CharacterTemplate& character,
	                      ResponseCB cb) const;

	void delete_character(std::uint32_t account_id,
	                      std::uint64_t id,
	                      ResponseCB cb) const;

	void rename_character(std::uint32_t account_id,
	                      std::uint64_t character_id,
	                      const utf8_string& name,
	                      RenameCB cb) const;
};

} // ember