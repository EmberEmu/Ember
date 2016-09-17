/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Patcher.h"
#include <shared/util/FileMD5.h>
#include <algorithm>
#include <fstream>
//#include <regex>
//#include <cstdio>

namespace ember {

Patcher::Patcher(std::vector<GameVersion> versions, std::vector<PatchMeta> patches)
                 : versions_(std::move(versions)), patches_(std::move(patches)),
                   survey_id_(0) {
	generate_graph();
}

void Patcher::generate_graph() {

}

boost::optional<FileMeta> Patcher::find_patch(const GameVersion& client_version) const {
	return boost::optional<FileMeta>();
}

auto Patcher::check_version(const GameVersion& client_version) const -> PatchLevel {
	if(std::find(versions_.begin(), versions_.end(), client_version) != versions_.end()) {
		return PatchLevel::OK;
	}

	// Figure out whether any of the allowed client versions are newer than the client.
	// If so, there's a chance that it can be patched.
	for(auto& v : versions_) {
		if(v > client_version) {
			return PatchLevel::TOO_OLD;
		}
	}

	return PatchLevel::TOO_NEW;
}

void Patcher::set_survey(const std::string& path, std::uint32_t id) {
	survey_.name = "Survey";
	survey_id_ = id;
	
	std::ifstream file(path + "Survey.mpq", std::ios::binary | std::ios::ate);

	if(!file.is_open()) {
		throw std::runtime_error("Unable to open " + path + "Survey.mpq");
	}

	survey_.size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(static_cast<std::size_t>(survey_.size));
	file.read(buffer.data(), survey_.size);

	if(!file.good()) {
		throw std::runtime_error("An error occured while reading " + path + "Survey.mpq");
	}

	auto md5 = util::generate_md5(buffer.data(), buffer.size());
	std::copy(md5.begin(), md5.end(), survey_.md5.data());
	survey_data_ = std::move(buffer);
}

FileMeta Patcher::survey_meta() const {
	return survey_;
}

bool Patcher::survey_platform(grunt::Platform platform, grunt::System os) const {
	if(platform != grunt::Platform::x86 || os != grunt::System::Windows) {
		return false;
	}

	return true;
}

const std::vector<char>& Patcher::survey_data(grunt::Platform platform, grunt::System os) const {
	if(!survey_platform(platform, os)) {
		throw std::invalid_argument("Attempted to retrieve survey binaries for an unsupported platform!");
	}

	return survey_data_;
}

std::uint32_t Patcher::survey_id() const {
	return survey_id_;
}

std::vector<PatchMeta> Patcher::load_patches(const std::string& path, const dal::PatchDAO& dao,
                                             log::Logger* logger) {
	auto patches = dao.fetch_patches();

	for(auto& patch : patches) {
		bool dirty = false;

		// we open each patch to make sure that it at least exists
		std::ifstream file(path + patch.file_meta.name, std::ios::binary | std::ios::ate);

		if(!file.is_open()) {
			throw std::runtime_error("Unable to open patch " + path + patch.file_meta.name);
		}

		if(patch.file_meta.size == 0) {
			patch.file_meta.size = static_cast<std::uint64_t>(file.tellg());

			if(!file.good()) {
				throw std::runtime_error("Unable determine patch size for " + path + patch.file_meta.name);
			}

			dirty = true;
		}

		int value = 0;

		for(auto c : patch.file_meta.md5) {
			value |= c;
		}

		if(!value) {
			LOG_INFO(logger) << "Calculating MD5 for " << patch.file_meta.name << LOG_SYNC;
			auto md5 = util::generate_md5(path + patch.file_meta.name);
			std::copy(md5.begin(), md5.end(), patch.file_meta.md5.data());
			dirty = true;
		}

		if(dirty) {
			dao.update(patch);
		}
	}

	return patches;
}

} // ember