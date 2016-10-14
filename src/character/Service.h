/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CharacterHandler.h"
#include "Character_generated.h"
#include <spark/Helpers.h>
#include <spark/Service.h>
#include <logger/Logging.h>
#include <shared/database/objects/Character.h>
#include <shared/database/daos/CharacterDAO.h>
#include <vector>

namespace ember {

class Service final : public spark::EventHandler {
	const CharacterHandler& handler_;
	dal::CharacterDAO& character_dao_;
	spark::Service& spark_;
	spark::ServiceDiscovery& discovery_;
	std::unordered_map<messaging::core::Opcode, spark::LocalDispatcher> handlers_;
	log::Logger* logger_;

	void retrieve_characters(const spark::Link& link, const spark::Message& message);
	void create_character(const spark::Link& link, const spark::Message& message);
	void rename_character(const spark::Link& link, const spark::Message& message);
	void delete_character(const spark::Link& link, const spark::Message& message);

	void send_character_list(const spark::Link& link, const spark::Beacon& token, 
	                         em::character::Status status, const std::vector<Character>& characters);

	void send_response(const spark::Link& link, const spark::Beacon& token, 
	                   messaging::character::Status status, protocol::Result result);

	void send_rename_response(const spark::Link& link, const spark::Beacon& token,
	                          protocol::Result result, boost::optional<Character> character);

public:
	Service(dal::CharacterDAO& character_dao, const CharacterHandler& handler, spark::Service& spark,
	        spark::ServiceDiscovery& discovery, log::Logger* logger);
	~Service();

	void on_message(const spark::Link& link, const spark::Message& message) override;
	void on_link_up(const spark::Link& link) override;
	void on_link_down(const spark::Link& link) override;
};

} // ember