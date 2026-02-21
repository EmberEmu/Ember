/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ports/Forward.h>
#include <istream>

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

} // ports, ember