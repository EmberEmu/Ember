/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "SchemaParser.h"
#include <flatbuffers/reflection.h>
#include <flatbuffers/reflection_generated.h>
#include <shared/util/FormatPacket.h>
#include <iostream>
namespace ember {

SchemaParser::SchemaParser(std::vector<char> buffer) : buffer_(std::move(buffer)) {
	verify();
}

void SchemaParser::verify() {
	flatbuffers::Verifier verifier(reinterpret_cast<std::uint8_t*>(buffer_.data()), buffer_.size());

	if(!reflection::VerifySchemaBuffer(verifier)) {
		throw std::runtime_error("Schema verification failed!");
	}
}

} // ember