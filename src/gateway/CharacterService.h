/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Config.h"
#include <spark/Service.h>
#include <spark/ServiceDiscovery.h>
#include <spark/temp/MessageRoot_generated.h>
#include <game_protocol/ResultCodes.h>
#include <logger/Logging.h>
#include <shared/database/objects/Character.h>
#include <botan/bigint.h>
#include <boost/uuid/uuid_generators.hpp>
#include <functional>
#include <memory>
#include <vector>
#include <cstdint>

namespace ember {

class CharacterService final : public spark::EventHandler {
public:
	typedef std::function<void(messaging::character::Status, boost::optional<protocol::ResultCode>)> ResponseCB;
	typedef std::function<void(messaging::character::Status, protocol::ResultCode, std::uint64_t, std::string)> RenameCB;
	typedef std::function<void(messaging::character::Status, std::vector<Character>)> RetrieveCB;

private:
	spark::Service& spark_;
	spark::ServiceDiscovery& s_disc_;
	log::Logger* logger_;
	std::unique_ptr<spark::ServiceListener> listener_;
	mutable boost::uuids::random_generator generate_uuid; // functor
	spark::Link link_;
	const Config& config_;
	
	void service_located(const messaging::multicast::LocateAnswer* message);

	void handle_reply(const spark::Link& link, const boost::uuids::uuid& uuid,
	                  boost::optional<const messaging::MessageRoot*> root,
	                  const ResponseCB& cb) const;

	void handle_retrieve_reply(const spark::Link& link, const boost::uuids::uuid& uuid,
	                           boost::optional<const messaging::MessageRoot*> root,
	                           const RetrieveCB& cb) const;

	void handle_rename_reply(const spark::Link& link, const boost::uuids::uuid& uuid,
	                         boost::optional<const messaging::MessageRoot*> root,
	                         const RenameCB& cb) const;

public:
	CharacterService(spark::Service& spark, spark::ServiceDiscovery& s_disc, const Config& config,
	                 log::Logger* logger);

	~CharacterService();

	void handle_message(const spark::Link& link, const messaging::MessageRoot* root) override;
	void handle_link_event(const spark::Link& link, spark::LinkState event) override;

	void retrieve_characters(std::uint32_t account_id, RetrieveCB cb) const;

	void create_character(std::uint32_t account_id, const CharacterTemplate& character,
	                      ResponseCB cb) const;

	void delete_character(std::uint32_t account_id, std::uint64_t id, ResponseCB cb) const;

	void rename_character(std::uint32_t account_id, std::uint64_t character_id,
	                      const std::string& name, RenameCB cb) const;
};

} // ember