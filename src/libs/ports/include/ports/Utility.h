/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/Forward.h>
#include <ports/Protocol.h>
#include <istream>
#include <ostream>

namespace ember::ports {

// for use by Boost Program Options
inline std::istream& operator>>(std::istream& in, Forward::Method& method) {
	std::string token;
	in >> token;

	if(token == "auto") {
		method = Forward::Method::auto_determine;
	} else if(token == "upnp") {
		method = Forward::Method::upnp;
	} else if(token == "natpmp") {
		method = Forward::Method::pmp_pcp;
	} else {
		in.setstate(std::ios_base::failbit);
	}

	return in;
}

// for use by Boost Program Options
inline std::istream& operator>>(std::istream& in, Protocol& protocol) {
	std::string token;
	in >> token;

	if(token == "udp") {
		protocol = Protocol::udp;
	} else if(token == "tcp") {
		protocol = Protocol::tcp;
	} else if(token == "all") {
		protocol = Protocol::all;
	} else {
		in.setstate(std::ios_base::failbit);
	}

	return in;
}

inline std::ostream& operator<<(std::ostream& stream, const Protocol& protocol) {
	switch(protocol) {
		case Protocol::all:
			stream << "all";
			break;
		case Protocol::tcp:
			stream << "tcp";
			break;
		case Protocol::udp:
			stream << "udp";
			break;
	}

	return stream;
}

} // ports, ember