/*
 * Copyright (c) 2024 - 2025 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <utility>

namespace ember::spark::io {

template<typename stream_type>
class stream_read_adaptor final {
	stream_type& _stream;

public:
	stream_read_adaptor(stream_type& stream) 
		: _stream(stream) {}

	void operator&(auto&& arg) {
		_stream >> arg;
	}

	template<typename ...Ts>
	void operator()(Ts&&... args) {
		(_stream >> ... >> args);
	}

	template<typename ...Ts>
	void forward(Ts&&... args) {
		_stream.get(std::forward<Ts>(args)...);
	}
};

template<typename stream_type>
class stream_write_adaptor final {
	stream_type& _stream;

public:
	stream_write_adaptor(stream_type& stream) 
		: _stream(stream) {}

	void operator&(auto&& arg) {
		_stream << arg;
	}

	template<typename ...Ts>
	void operator()(Ts&&... args) {
		(_stream << ... << args);
	}

	template<typename ...Ts>
	void forward(Ts&&... args) {
		_stream.put(std::forward<Ts>(args)...);
	}
};

} // io, spark, ember