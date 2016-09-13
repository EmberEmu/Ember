/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "GameVersion.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <botan/bigint.h>
#include <string>
#include <sstream>
#include <cstdint>

namespace ember {

struct FileMeta {
	std::string name;
	std::string rel_path;
	Botan::BigInt md5;
	std::uint64_t size;
};

struct PatchMeta {
	std::uint16_t build_from;
	std::uint16_t build_to;
	FileMeta file_meta;
};

} // ember

namespace boost { namespace serialization {

template<class Archive>
void serialize(Archive & ar, ember::FileMeta& fm, const unsigned int version) {
    ar & fm.name;
	ar & fm.rel_path;
    ar & fm.md5;
    ar & fm.size;
}

template<class Archive>
void serialize(Archive & ar, ember::PatchMeta& pm, const unsigned int version) {
    ar & pm.build_from;
    ar & pm.build_to;
    ar & pm.file_meta;
}

template<class Archive>
void serialize(Archive & ar, Botan::BigInt& bi, const unsigned int version) {
	std::stringstream ss;
	ss << bi;
    ar & ss.str();
}

}} // serialization, boost