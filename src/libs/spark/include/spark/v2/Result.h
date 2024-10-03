/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace ember::spark::v2 {

enum class Result {
	OK,
	LINK_GONE,
	TIMED_OUT,
	NET_ERROR,
	WRONG_MESSAGE_TYPE
};

} // v2, spark, ember