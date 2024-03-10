/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace ember::ports::upnp {

enum class ErrorCode {
	SUCCESS                    = 0x00,
	NETWORK_FAILURE            = 0x01,
	SOAP_ARGUMENTS_MISMATCH    = 0x02,
	SOAP_MISSING_URI           = 0x03,
	SOAP_NO_ARGUMENTS          = 0x04,
	HTTP_BAD_RESPONSE          = 0x80,
	HTTP_BAD_HEADERS           = 0x81,
	HTTP_NOT_OK                = 0x82,
	HTTP_HEADER_FIELD_AWOL     = 0x83,
};

} // upnp, ports, ember