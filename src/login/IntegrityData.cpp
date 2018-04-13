/*
 * Copyright (c) 2016 - 2018 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "IntegrityData.h"
#include <shared/util/FNVHash.h>
#include <filesystem>
#include <fstream>
#include <cstddef>

namespace fs = std::filesystem;

namespace ember {

IntegrityData::IntegrityData(const std::vector<GameVersion>& versions, const std::string& path) {
	std::initializer_list<std::string_view> winx86 { "WoW.exe", "fmod.dll", "ijl15.dll",
	                                                 "dbghelp.dll", "unicows.dll" };

	std::initializer_list<std::string_view> macx86 { "MacOS/World of Warcraft", "Info.plist",
	                                                 "Resources/Main.nib/objects.xib",
	                                                 "Resources/wow.icns", "PkgInfo" };

	std::initializer_list<std::string_view> macppc { "MacOS/World of Warcraft", "Info.plist",
	                                                 "Resources/Main.nib/objects.xib",
	                                                 "Resources/wow.icns", "PkgInfo" };
	
	for(auto& version : versions) {
		load_binaries(path, version.build, winx86, grunt::System::Win, grunt::Platform::x86);
		load_binaries(path, version.build, macx86, grunt::System::OSX, grunt::Platform::x86);
		load_binaries(path, version.build, macppc, grunt::System::OSX, grunt::Platform::PPC);
	}

	// ensure we have at least one supported client
	if(data_.empty()) {
		throw std::runtime_error("Client integrity checking is enabled but no binaries were found");
	}
}

std::optional<const std::vector<std::byte>*>
IntegrityData::lookup(GameVersion version, grunt::Platform platform, grunt::System os) const {
	auto it = data_.find(hash(version.build, platform, os));

	if(it == data_.end()) {
		return std::nullopt;
	}
	
	return &it->second;
}

void IntegrityData::load_binaries(const std::string& path, std::uint16_t build,
                                  const std::initializer_list<std::string_view>& files,
                                  grunt::System system, grunt::Platform platform) {
	fs::path dir(path + grunt::to_string(system) + "_" + grunt::to_string(platform)
	              + "_" + std::to_string(build));

	if(!fs::is_directory(dir)) {
		return;
	}

	auto fnv = hash(build, platform, system);
	std::vector<std::byte> buffer;
	std::size_t write_offset = 0;

	// write all of the binary data into a single buffer
	for(auto& f : files) {
		fs::path fpath = dir;
		fpath /= f.data();

		std::ifstream file(fpath.string(), std::ios::binary | std::ios::ate);

		if(!file.is_open()) {
			throw std::runtime_error("Unable to open " + fpath.string());
		}

		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);
		buffer.resize(static_cast<std::size_t>(size) + buffer.size());

		if(!file.read(reinterpret_cast<char*>(buffer.data()) + write_offset, size)) {
			throw std::runtime_error("Unable to read " + fpath.string());
		}

		write_offset += static_cast<std::size_t>(size);
	}

	data_.emplace(fnv, std::move(buffer));
}

std::size_t IntegrityData::hash(std::uint16_t build, grunt::Platform platform,
                                grunt::System os) const {
	FNVHash hasher;
	hasher.update(build);
	hasher.update(platform);
	return hasher.update(os);
}

} // ember
