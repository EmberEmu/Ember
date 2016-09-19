/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "GameVersion.h"
#include "PatchGraph.h"
#include "grunt/Magic.h"
#include <shared/database/daos/PatchDAO.h>
#include <shared/database/objects/PatchMeta.h>
#include <logger/Logging.h>
#include <boost/optional.hpp>
#include <string>
#include <vector>

namespace ember {

class Patcher {
	const std::vector<PatchMeta> patches_;
	const std::vector<GameVersion> versions_;
	std::unordered_map<std::size_t, PatchGraph> graphs_;
	std::unordered_map<std::size_t, std::vector<PatchMeta>> patch_bins;

	FileMeta survey_;
	std::vector<char> survey_data_;
	std::uint32_t survey_id_;

public:
	enum class PatchLevel { OK, TOO_OLD, PATCH_AVAILABLE, TOO_NEW };

	Patcher(std::vector<GameVersion> versions, std::vector<PatchMeta> patches);
	
	// Survey
	void set_survey(const std::string& path, std::uint32_t id);
	FileMeta survey_meta() const;
	std::uint32_t survey_id() const;
	bool Patcher::survey_platform(grunt::Platform platform, grunt::System os) const;
	const std::vector<char>& survey_data(grunt::Platform platform, grunt::System os) const;

	// Patching
	boost::optional<PatchMeta> find_patch(const GameVersion& client_version, grunt::Locale locale,
	                                      grunt::Platform platform, grunt::System os) const;

	PatchLevel check_version(const GameVersion& client_version) const;

	static std::vector<PatchMeta> load_patches(const std::string& path, const dal::PatchDAO& dao,
	                                           log::Logger* logger);
};

} // ember