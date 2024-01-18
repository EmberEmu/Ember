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
	{ Attributes::MAPPED_ADDRESS,     RFCMode::RFC_BOTH },
	{ Attributes::RESPONSE_ADDRESS,   RFCMode::RFC3489  },
	{ Attributes::CHANGE_REQUEST,     RFCMode::RFC3489  },
	{ Attributes::SOURCE_ADDRESS,     RFCMode::RFC3489  },
	{ Attributes::CHANGED_ADDRESS,    RFCMode::RFC3489  },
	{ Attributes::USERNAME,           RFCMode::RFC_BOTH },
	{ Attributes::PASSWORD,           RFCMode::RFC3489  },
	{ Attributes::MESSAGE_INTEGRITY,  RFCMode::RFC_BOTH },
	{ Attributes::ERROR_CODE,         RFCMode::RFC_BOTH },
	{ Attributes::UNKNOWN_ATTRIBUTES, RFCMode::RFC_BOTH },
	{ Attributes::REFLECTED_FROM,     RFCMode::RFC3489  },
	{ Attributes::REALM,              RFCMode::RFC5389  },
	{ Attributes::NONCE,              RFCMode::RFC5389  },
	{ Attributes::XOR_MAPPED_ADDRESS, RFCMode::RFC5389  },
	{ Attributes::OTHER_ADDRESS,      RFCMode::RFC_BOTH }
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
	{ Attributes::RESPONSE_ORIGIN,              MessageType::BINDING_RESPONSE       },
	{ Attributes::MESSAGE_INTEGRITY_SHA256,     MessageType::BINDING_RESPONSE       },
	{ Attributes::FINGERPRINT,                  MessageType::BINDING_RESPONSE       },
};

} // stun, ember