/*
 * Copyright (c) 2024 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Utility.h"
#include <logger/Logger.h>
#include <algorithm>
#include <random>
#include <string_view>

namespace ember {

const std::string_view random_tip(const dbc::Store<dbc::GameTips>& tips) {
	if(!tips.size()) {
		return {};
	}

	// select a random tip
	std::mt19937 gen{std::random_device{}()};
	std::uniform_int_distribution<> index(0, std::ranges::distance(tips.values()) - 1);
	auto it = tips.values().begin();
	std::advance(it, index(gen));
	const auto& game_tip = *it;

	// trim any leading formatting that we can't make use of
	std::string_view text(game_tip.text.en_gb);
	std::string_view needle("|cffffd100Tip:|r ");
	
	if(text.find(needle) != text.npos) {
		text = text.substr(needle.size(), text.size());
	}

	// trim any trailing newlines that we don't want to print
	if(auto pos = text.find("\r\n"); pos != text.npos) {
		text = text.substr(0, pos);
	}

	return text;
}

bool validate_maps(std::span<const std::int32_t> maps, const dbc::Store<dbc::Map>& dbc, log::Logger& logger) {
	const auto validate = [&](const auto id) {
		auto it = std::ranges::find_if(dbc, [&](const auto& record) {
			return record.second.id == id;
		});

		if(it == dbc.end()) {
			SLOG_ERROR(logger, "Unknown map ID ({}) specified", id);
			return false;
		}

		auto& [_, map] = *it;

		if(map.instance_type != dbc::Map::InstanceType::NORMAL) {
			SLOG_ERROR(logger, "Map {} ({}) is not an open world area", map.id, map.map_name.en_gb);
			return false;
		}

		return true;
	};

	return !std::ranges::contains(maps, false, validate);
}

void print_maps(std::span<const std::int32_t> maps, const dbc::Store<dbc::Map>& dbc, log::Logger& logger) {
	for(auto id : maps) {
		SLOG_INFO(logger, " - {}", dbc[id]->map_name.en_gb);
	}
}

} // ember