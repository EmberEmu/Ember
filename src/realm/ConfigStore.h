/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Config.h"
#include <mutex>
#include <optional>

namespace ember::realm {

class ConfigStore final {
	static inline thread_local std::optional<Config> tls_config_;

	mutable Config base_config_;
	mutable std::mutex lock;

public:
	explicit ConfigStore(Config config) {
		update_default_config(std::move(config));
	}

	ConfigStore(const ConfigStore&) = delete;
	ConfigStore& operator=(const ConfigStore&) = delete;

	/*
	 * Updates the config that will be served by threads that have not called
	 * update_tls_config (i.e. those outside of the Asio service pool).
	 * 
	 * This only exists to serve as a fallback mechanism in case other threads
	 * require the use of updated configs in the future.
	 */
	void update_default_config(Config config) {
		std::lock_guard guard(lock);
		base_config_ = std::move(config);
		tls_config_ = base_config_;
	}

	/*
	 * Updates the config for the current thread only. 
	 */
	void update_thread_config(Config config) {
		tls_config_ = std::move(config);
	}

	/*
	 * Retrieves the cached config copy for the calling thread, falling back to the
	 * base/default config if the calling thread has never called update_thead_config.
	 */
	const Config& config() const {
		if(tls_config_) [[likely]] {
			return *tls_config_;
		} else {
			std::lock_guard guard(lock);
			return base_config_;
		}
	}
};

} // realm, ember