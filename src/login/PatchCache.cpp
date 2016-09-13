/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "PatchCache.h"
#include <shared/util/FileMD5.h>
#include <logger/Logging.h>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <botan/md5.h>

namespace bfs = boost::filesystem;

namespace ember { namespace patch_cache {

std::vector<PatchMeta> load() {
	std::vector<PatchMeta> patches;
	std::fstream file("patch_md5_cache", std::ios::in | std::ios::binary);

	if(!file.is_open()) {
		return patches;
	}

	std::size_t entries;
	file.read(reinterpret_cast<char*>(&entries), sizeof(entries));
	boost::archive::text_iarchive archive(file);

	for(std::size_t i = 0; i < entries; ++i) {
		PatchMeta patch;
		archive >> patch;

		if(!file.good()) {
			LOG_WARN_GLOB << "Patch cache read error" << LOG_SYNC;
			break;
		}

		patches.emplace_back(std::move(patch));
	}

	return patches;
}

void save(const std::vector<PatchMeta>& patches) {
	std::fstream file("patch_md5_cache", std::ios::out | std::ios::binary | std::ios::trunc);

	if(!file.is_open()) {
		LOG_WARN_GLOB << "Unable to open patch cache for writing" << LOG_SYNC;
		return;
	}

	std::size_t size = patches.size();
	file.write(reinterpret_cast<char*>(&size), sizeof(size));
	boost::archive::text_oarchive archive(file);

	for(auto& patch : patches) {
		archive << patch;
	}

	if(!file.good()) {
		file.close();
		std::remove("patch_md5_cache");
		LOG_WARN_GLOB << "Unable to write patch cache" << LOG_SYNC;
	}
}

std::vector<PatchMeta> fetch(const std::string& path) {
	bfs::path dir(path);
	std::vector<PatchMeta> patches;
	std::vector<PatchMeta> cached(load());
	bool cache_dirty = false;

    if(bfs::is_directory(dir)) {
		for(auto& entry : boost::make_iterator_range(bfs::directory_iterator(dir))) {
			const auto& path = entry.path();

			if(path.extension() != ".mpq") {
				continue;
			}

			std::string name = path.filename().string();
			
			auto it = std::find_if(cached.begin(), cached.end(), [&](const PatchMeta& m) {
				return name == m.file_meta.name;
			});

			if(it == cached.end()) {
				PatchMeta patch;
				std::string rel_path = path.relative_path().string();
				patch.file_meta.name = std::move(name);
				patch.file_meta.md5 = util::generate_md5(rel_path);
				patch.file_meta.rel_path = std::move(rel_path);
				cache_dirty = true;
				patches.emplace_back(std::move(patch));
			} else {
				patches.emplace_back(std::move(*it));
			}
		}
    }

	if(cache_dirty) {
		save(patches);
	}

	return patches;
}

}} // patch_cache, ember