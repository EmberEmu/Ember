/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "World.h"
#include <dbcreader/DBCReader.h>
#include <boost/program_options.hpp>
#include <algorithm>
#include <span>
#include <string_view>
#include <vector>
#include <cstdlib>

namespace po = boost::program_options;

namespace ember::world {

bool validate_maps(std::span<const std::int32_t> maps,
                   const dbc::DBCMap<dbc::Map>& dbc,
                   log::Logger& logger);

void print_maps(std::span<const std::int32_t> maps,
                const dbc::DBCMap<dbc::Map>& dbc,
                log::Logger& logger);

int launch(const boost::program_options::variables_map& args, log::Logger& logger) {
	LOG_INFO(logger) << "Loading DBC data..." << LOG_SYNC;

	dbc::DiskLoader loader(args["dbc.path"].as<std::string>(), [&](auto message) {
		LOG_DEBUG(logger) << message << LOG_SYNC;
	});

	auto dbc_store = loader.load("Map");

	LOG_INFO(logger) << "Resolving DBC references..." << LOG_SYNC;
	dbc::link(dbc_store);

	const auto& maps = args["world.map_id"].as<std::vector<std::int32_t>>();

	if(!validate_maps(maps, dbc_store.map, logger)) {
		return EXIT_FAILURE;
	}

	print_maps(maps, dbc_store.map, logger);
	return EXIT_SUCCESS;
}

bool validate_maps(std::span<const std::int32_t> maps,
                   const dbc::DBCMap<dbc::Map>& dbc,
                   log::Logger& logger) {
	for(auto id : maps) {
		auto it = std::ranges::find_if(dbc, [&](auto& record) {
			return record.second.id == id;
		});

		if(it == dbc.end()) {
			LOG_ERROR_SYNC(logger, "Unknown map ID ({}) specified", id);
			return false;
		}

		auto& [_, map] = *it;

		if(map.instance_type != dbc::Map::InstanceType::NORMAL) {
			LOG_ERROR_SYNC(logger, "Map {} ({}) is not an open world area", map.id, map.internal_name);
			return false;
		}
	}

	return true;
}

void print_maps(std::span<const std::int32_t> maps,
                const dbc::DBCMap<dbc::Map>& dbc,
                log::Logger& logger) {
	LOG_INFO_SYNC(logger, "Serving as world server for maps:");

	for(auto id : maps) {
		LOG_INFO_SYNC(logger, " - {}", dbc[id]->internal_name);
	}
}

po::options_description options() {
	po::options_description opts;
	opts.add_options()
		("database.min_connections", po::value<unsigned short>()->required())
		("database.max_connections", po::value<unsigned short>()->required())
		("database.config_path", po::value<std::string>()->required())
		("network.interface", po::value<std::string>()->required())
		("network.port", po::value<std::uint16_t>()->required())
		("network.tcp_no_delay", po::value<bool>()->required())
		("dbc.path", po::value<std::string>()->required())
		("spark.address", po::value<std::string>()->required())
		("spark.port", po::value<std::uint16_t>()->required())
		("spark.multicast_interface", po::value<std::string>()->required())
		("spark.multicast_group", po::value<std::string>()->required())
		("spark.multicast_port", po::value<std::uint16_t>()->required())
		("world.id", po::value<std::uint32_t>()->required())
		("world.map_id", po::value<std::vector<std::int32_t>>()->required());
	return opts;
}

} // world, ember