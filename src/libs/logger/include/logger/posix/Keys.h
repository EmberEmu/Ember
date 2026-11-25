/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <shared/utility/enum_bitmask.h>
#include <utility>

namespace ember::log::posix {

enum class Key {
	character,
	enter,
	tab,
	escape,
	backspace,
	del,
	up,
	down,
	left,
	right,
	home,
	end,
	other,
	interrupted,
	end_of_input
};

enum class KeyModifier {
	none = 0,
	alt = 1,
	shift = 2,
	ctrl = 4
};

struct KeyEvent {
	Key key = Key::other;
	unsigned char ch = 0;
	KeyModifier modifier = KeyModifier::none;
};

} // posix, log, ember

namespace ember {

ENABLE_BITMASK(log::posix::KeyModifier);

namespace log::posix {

inline constexpr bool has_modifier(KeyModifier modifiers, KeyModifier modifier) {
	return (modifiers & modifier) != KeyModifier::none;
}

inline constexpr KeyModifier modifier_from_xterm(const unsigned param) {
	KeyModifier result = KeyModifier::none;

	if(param < 2) {
		return result;
	}

	const unsigned bits = param - 1;

	if(bits & 1) {
		result |= KeyModifier::shift;
	}

	if(bits & 2) {
		result |= KeyModifier::alt;
	}

	if(bits & 4) {
		result |= KeyModifier::ctrl;
	}

	if(bits & 8) {
		result |= KeyModifier::alt; // meta
	}

	return result;
}

}} // posix, log, ember