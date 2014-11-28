/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstddef>

namespace ember { namespace connection_pool {

class CheckoutClean {
protected:
	bool return_clean() const {
		return false;
	}

	bool sched_clean() const {
		return true;
	}
};

class CheckinClean {
protected:
	bool return_clean() const {
		return true;
	}

	bool sched_clean() const {
		return false;
	}
};

class FixedSize {
protected:
	std::size_t grow(std::size_t size, std::size_t max) const {
		throw std::exception("Database pool ran out of connections!");
	}
};

class AggressiveGrowth {
protected:
	std::size_t grow(std::size_t size, std::size_t max) const {
		if(max != 0 && size >= max) {
			throw std::exception("Database pool ran out of connections!");
		}

		std::size_t available = max - size;
		std::size_t desired = size / 2;

		if(max != 0 && desired > available) {
			desired = available;
		}

		return desired;
	}
};

class ExponentialGrowth {
protected:
	std::size_t grow(std::size_t size, std::size_t max) const {
		if(max != 0 && size >= max) {
			throw std::exception("Database pool ran out of connections!");
		}

		std::size_t available = max - size;
		std::size_t desired = size * 2;

		if(max != 0 && desired > available) {
			desired = available;
		}

		return desired;
	}
};

class LinearGrowth {
protected:
	std::size_t grow(std::size_t size, std::size_t max) const {
		if(max != 0 && size >= max) {
			throw std::exception("Database pool ran out of connections!");
		}

		return 1;
	}
};

class NoReuse {

};

class AlwaysReuse {

};

}} //connection_pool, ember