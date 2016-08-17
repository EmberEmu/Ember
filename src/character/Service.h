/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CharacterHandler.h"
#include <shared/database/daos/CharacterDAO.h>
#include <spark/Service.h>
#include <spark/temp/Character_generated.h>
#include <spark/temp/MessageRoot_generated.h>
#include <logger/Logging.h>
#include <string>
#include <cstdint>

namespace ember {

class Service final : public spark::EventHandler {
	const CharacterHandler& handler_;
	dal::CharacterDAO& character_dao_;
	spark::Service& spark_;
	spark::ServiceDiscovery& discovery_;
	log::Logger* logger_;

	void retrieve_characters(const spark::Link& link, const messaging::MessageRoot* root);
	void create_character(const spark::Link& link, const messaging::MessageRoot* root);
	void rename_character(const spark::Link& link, const messaging::MessageRoot* root);
	void delete_character(const spark::Link& link, const messaging::MessageRoot* root);

	void send_character_list(const spark::Link& link, const std::vector<std::uint8_t>& tracking,
	                         const boost::optional<std::vector<Character>>& characters);

	void send_response(const spark::Link& link, const messaging::MessageRoot* root,
	                   messaging::character::Status status, protocol::ResultCode result);

	void send_response(const spark::Link& link, const std::vector<std::uint8_t>& tracking,
	                   messaging::character::Status status, protocol::ResultCode result);

	void send_rename_response(const spark::Link& link, const std::vector<std::uint8_t>& tracking,
	                          messaging::character::Status status, protocol::ResultCode result,
	                          boost::optional<Character> character);

public:
	Service(dal::CharacterDAO& character_dao, const CharacterHandler& handler, spark::Service& spark,
	        spark::ServiceDiscovery& discovery, log::Logger* logger);
	~Service();

	void handle_message(const spark::Link& link, const messaging::MessageRoot* msg) override;
	void handle_link_event(const spark::Link& link, spark::LinkState event) override;
};

} // ember