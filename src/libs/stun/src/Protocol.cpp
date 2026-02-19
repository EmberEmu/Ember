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
	{ Attributes::mapped_address,     { rfc3489, rfc5389, rfc5780, rfc8445 }},
	{ Attributes::response_address,   { rfc3489 }},
	{ Attributes::change_request,     { rfc3489, rfc5780, rfc8445 }},
	{ Attributes::source_address,     { rfc3489 }},
	{ Attributes::changed_address,    { rfc3489 }},
	{ Attributes::username,           { rfc3489, rfc5389, rfc5780, rfc8445 }},
	{ Attributes::password,           { rfc3489 }},
	{ Attributes::message_integrity,  { rfc3489, rfc5389, rfc5780, rfc8445 }},
	{ Attributes::error_code,         { rfc3489, rfc5389, rfc5780, rfc8445 }},
	{ Attributes::unknown_attributes, { rfc3489, rfc5389, rfc5780, rfc8445 }},
	{ Attributes::reflected_from,     { rfc3489 }},
	{ Attributes::realm,              { rfc5389, rfc5780, rfc8445 }},
	{ Attributes::nonce,              { rfc5389, rfc5780, rfc8445 }},
	{ Attributes::xor_mapped_address, { rfc5389, rfc5780, rfc8445 }},
	{ Attributes::other_address,      { rfc3489, rfc5389, rfc5780, rfc8445 }},
	{ Attributes::padding,            { rfc5780, rfc8445 }},
	{ Attributes::response_port,      { rfc5780, rfc8445 }},
	{ Attributes::priority,           { rfc8445 }},
	{ Attributes::use_candidate,      { rfc8445 }}
};

// we don't handle shared secret types, YAGNI
AttrValidIn attr_valid_lut {
	{ Attributes::mapped_address,               MessageType::binding_response       },
	{ Attributes::source_address,               MessageType::binding_response       },
	{ Attributes::changed_address,              MessageType::binding_response       },
	{ Attributes::password,                     MessageType::binding_response       },
	{ Attributes::message_integrity,            MessageType::binding_response       },
	{ Attributes::error_code,                   MessageType::binding_error_response },
	{ Attributes::unknown_attributes,           MessageType::binding_error_response },
	{ Attributes::reflected_from,               MessageType::binding_response       },
	{ Attributes::realm,                        MessageType::binding_response       },
	{ Attributes::nonce,                        MessageType::binding_response       },
	{ Attributes::xor_mapped_address,           MessageType::binding_response       },
	{ Attributes::xor_mapped_addr_opt,          MessageType::binding_response       },
	{ Attributes::other_address,                MessageType::binding_response       },
	{ Attributes::response_origin,              MessageType::binding_response       },
	{ Attributes::message_integrity_sha256,     MessageType::binding_response       },
	{ Attributes::fingerprint,                  MessageType::binding_response       },
	{ Attributes::software,                     MessageType::binding_response       },
	{ Attributes::nonce,                        MessageType::binding_response       },
	{ Attributes::realm,                        MessageType::binding_response       },
	{ Attributes::padding,                      MessageType::binding_response       },
	{ Attributes::username,                     MessageType::binding_response       }
};

} // stun, ember