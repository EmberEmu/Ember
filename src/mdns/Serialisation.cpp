/*
 * Copyright (c) 2021 - 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Serialisation.h"
#include <spark/buffers/pmr/BufferAdaptor.h>
#include <logger/Logger.h>
#include <gsl/gsl_util>
#include <boost/endian.hpp>

namespace be = boost::endian;

namespace ember::dns {

std::expected<Query, parser::Result> deserialise(std::span<const std::uint8_t> buffer) try {
	if(buffer.size() > MAX_DGRAM_LEN) {
		return std::unexpected(parser::Result::PAYLOAD_TOO_LARGE);
	}

	spark::io::pmr::BufferReadAdaptor adaptor(buffer);
	spark::io::pmr::BinaryStreamReader stream(adaptor);

	Query query;

	parser::ParseContext ctx {
		.buffer = buffer,
		.stream = stream,
	};

	parser::parse_header(query, stream);
	parser::parse_records(query, ctx);

	if(!stream) {
		return std::unexpected(parser::Result::STREAM_ERROR);
	}

	return query;
} catch(parser::Result& r) {
	return std::unexpected(r);
}

void serialise(const Query& query, spark::io::pmr::BinaryStream& stream) {
	parser::write_header(query, stream);
	const auto ptrs = parser::write_questions(query, stream);
	parser::write_resource_records(query, ptrs, stream);
}

} // dns, ember