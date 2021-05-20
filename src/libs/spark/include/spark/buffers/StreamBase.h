/*
 * Copyright (c) 2021 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace ember::spark {

template<typename T>
concept trivially_copyable = std::is_trivially_copyable<T>::value;

enum class StreamStateBase {
	OK, READ_LIMIT_ERR, BUFF_LIMIT_ERR
};

} // spark, ember
