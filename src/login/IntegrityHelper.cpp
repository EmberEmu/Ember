/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "IntegrityHelper.h"
#include <shared/util/FNVHash.h>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;
using namespace std::string_literals;

namespace ember {

IntegrityHelper::IntegrityHelper(const std::vector<GameVersion>& versions, const std::string& path) {
	std::initializer_list<std::string> winx86 { "WoW.exe"s, "fmod.dll"s, "ijl15.dll"s, "dbghelp.dll"s, "unicows.dll"s };
	std::initializer_list<std::string> macx86 {  };
	std::initializer_list<std::string> macppc {  };

	for(auto& version : versions) {
		load_integrity_binaries(version.build, path, winx86, grunt::System::Win, grunt::Platform::x86);
		load_integrity_binaries(version.build, path, macx86, grunt::System::OSX, grunt::Platform::x86);
		load_integrity_binaries(version.build, path, macppc, grunt::System::OSX, grunt::Platform::PPC);
	}

	// ensure we have at least one supported client
	if(checkers_.empty()) {
		throw std::runtime_error("Client integrity checking is enabled but no binaries were found");
	}
}

const ExecutableChecksum* IntegrityHelper::checker(GameVersion version, grunt::Platform platform,
                                                   grunt::System os) const {
	auto it = checkers_.find(hash(version.build, platform, os));

	if(it == checkers_.end()) {
		return nullptr;
	}
	
	return &it->second;
}

void IntegrityHelper::load_integrity_binaries(std::uint16_t build, const std::string& path,
                                              const std::initializer_list<std::string>& bins,
                                              grunt::System system, grunt::Platform platform) {
	bfs::path dir(path + grunt::to_string(system) + "_" + grunt::to_string(platform)
	              + "_" + std::to_string(build) + "\\");

	if(bfs::is_directory(dir)) {
		auto fnv = hash(build, platform, system);
		ExecutableChecksum checksum(dir.string(), bins);
		checkers_.emplace(fnv, std::move(checksum));
	}
}

std::size_t IntegrityHelper::hash(std::uint16_t build, grunt::Platform platform,
                                  grunt::System os) const {
	FNVHash hasher;
	hasher.update(build);
	hasher.update(grunt::to_string(platform));
	return hasher.update(grunt::to_string(os));
}

} // ember