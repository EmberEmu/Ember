/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Packets.h"
#include <concepts>

namespace ember::grunt {

template<std::derived_from<Packet> _ty>
constexpr bool validate_opcode(Opcode opcode) {
	using enum Opcode;

	// done this way so it's only a single conditional when compiled
	if constexpr(std::same_as<_ty, client::LoginChallenge>) {
		return opcode == cmd_auth_logon_challenge
			|| opcode == cmd_auth_reconnect_challenge;
	} else if constexpr(std::same_as<_ty, client::LoginProof>) {
		return opcode == cmd_auth_logon_proof;
	} else if constexpr(std::same_as<_ty, client::ReconnectChallenge>) {
		return opcode == cmd_auth_reconnect_challenge;
	} else if constexpr(std::same_as<_ty, client::ReconnectProof>) {
		return opcode == cmd_auth_reconnect_proof;
	} else if constexpr(std::same_as<_ty, client::RequestRealmList>) {
		return opcode == cmd_realm_list;
	} else if constexpr(std::same_as<_ty, client::TransferAccept>) {
		return opcode == cmd_xfer_accept;
	} else if constexpr(std::same_as<_ty, client::TransferCancel>) {
		return opcode == cmd_xfer_cancel;
	} else if constexpr(std::same_as<_ty, client::TransferResume>) {
		return opcode == cmd_xfer_resume;
	} else if constexpr(std::same_as<_ty, client::SurveyResult>) {
		return opcode == cmd_survey_result;
	} else {
		return false;
	}
}

template<std::derived_from<Packet> _ty>
auto& packet_cast(Packet& packet) {
	if(!validate_opcode<_ty>(packet.opcode)) {
		throw grunt::bad_packet("packet opcode did not match type");
	}

	return packet.as<_ty>();
}

template<std::derived_from<Packet> _ty>
auto& packet_cast(const Packet& packet) {
	if(!validate_opcode<_ty>(packet.opcode)) {
		throw grunt::bad_packet("packet opcode did not match type");
	}

	return packet.as<const _ty>();
}

} // grunt, ember