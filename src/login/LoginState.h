/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace ember {

enum class LoginState {
	challenge,
	proof,
	reconnect_proof,
	request_realms,

	survey_initiate,
	survey_transfer,
	survey_result,

	patch_initiate,
	patch_transfer,

	fetching_user_login,
	fetching_user_reconnect,
	fetching_session,
	fetching_character_data,

	writing_session,
	writing_survey,

	closed
};

} // ember