/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stun/Protocol.h>
namespace ember::stun {

AttrReqBy attr_req_lut {
	{ Attributes::MAPPED_ADDRESS,     { RFC3489, RFC5389, RFC5780, RFC8445 }},
	{ Attributes::RESPONSE_ADDRESS,   { RFC3489 }},
	{ Attributes::CHANGE_REQUEST,     { RFC3489, RFC5780, RFC8445 }},
	{ Attributes::SOURCE_ADDRESS,     { RFC3489 }},
	{ Attributes::CHANGED_ADDRESS,    { RFC3489 }},
	{ Attributes::USERNAME,           { RFC3489, RFC5389, RFC5780, RFC8445 }},
	{ Attributes::PASSWORD,           { RFC3489 }},
	{ Attributes::MESSAGE_INTEGRITY,  { RFC3489, RFC5389, RFC5780, RFC8445 }},
	{ Attributes::ERROR_CODE,         { RFC3489, RFC5389, RFC5780, RFC8445 }},
	{ Attributes::UNKNOWN_ATTRIBUTES, { RFC3489, RFC5389, RFC5780, RFC8445 }},
	{ Attributes::REFLECTED_FROM,     { RFC3489 }},
	{ Attributes::REALM,              { RFC5389, RFC5780, RFC8445 }},
	{ Attributes::NONCE,              { RFC5389, RFC5780, RFC8445 }},
	{ Attributes::XOR_MAPPED_ADDRESS, { RFC5389, RFC5780, RFC8445 }},
	{ Attributes::OTHER_ADDRESS,      { RFC3489, RFC5389, RFC5780, RFC8445 }},
	{ Attributes::PADDING,            { RFC5780, RFC8445 }},
	{ Attributes::RESPONSE_PORT,      { RFC5780, RFC8445 }},
	{ Attributes::PRIORITY,           { RFC8445 }},
	{ Attributes::USE_CANDIDATE,      { RFC8445 }}
};

// we don't handle shared secret types, YAGNI
AttrValidIn attr_valid_lut {
	{ Attributes::MAPPED_ADDRESS,               MessageType::BINDING_RESPONSE       },
	{ Attributes::SOURCE_ADDRESS,               MessageType::BINDING_RESPONSE       },
	{ Attributes::CHANGED_ADDRESS,              MessageType::BINDING_RESPONSE       },
	{ Attributes::PASSWORD,                     MessageType::BINDING_RESPONSE       },
	{ Attributes::MESSAGE_INTEGRITY,            MessageType::BINDING_RESPONSE       },
	{ Attributes::ERROR_CODE,                   MessageType::BINDING_ERROR_RESPONSE },
	{ Attributes::UNKNOWN_ATTRIBUTES,           MessageType::BINDING_ERROR_RESPONSE },
	{ Attributes::REFLECTED_FROM,               MessageType::BINDING_RESPONSE       },
	{ Attributes::REALM,                        MessageType::BINDING_RESPONSE       },
	{ Attributes::NONCE,                        MessageType::BINDING_RESPONSE       },
	{ Attributes::XOR_MAPPED_ADDRESS,           MessageType::BINDING_RESPONSE       },
	{ Attributes::XOR_MAPPED_ADDR_OPT,          MessageType::BINDING_RESPONSE       },
	{ Attributes::OTHER_ADDRESS,                MessageType::BINDING_RESPONSE       },
	{ Attributes::RESPONSE_ORIGIN,              MessageType::BINDING_RESPONSE       },
	{ Attributes::MESSAGE_INTEGRITY_SHA256,     MessageType::BINDING_RESPONSE       },
	{ Attributes::FINGERPRINT,                  MessageType::BINDING_RESPONSE       },
	{ Attributes::SOFTWARE,                     MessageType::BINDING_RESPONSE       },
	{ Attributes::NONCE,                        MessageType::BINDING_RESPONSE       },
	{ Attributes::REALM,                        MessageType::BINDING_RESPONSE       },
	{ Attributes::PADDING,                      MessageType::BINDING_RESPONSE       },
	{ Attributes::USERNAME,                     MessageType::BINDING_RESPONSE       }
};

} // stun, ember