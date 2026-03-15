/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Config.h"
#include <thread/ServicePool.h>
#include <boost/asio/post.hpp>

namespace ember::realm {

class ConfigStore {
	static inline thread_local Config live_config_;
	static inline thread_local const Config* tls_config_ = nullptr;

	const thread::ServicePool& pool_;
	const Config base_config_;

public:
	ConfigStore(Config config, const thread::ServicePool& pool)
		: pool_(pool)
		, base_config_(std::move(config)) {
		post_config(base_config_);
	}

	void post_config(const Config& config) {
		for(auto& service : pool_) {
			boost::asio::post(*service, [&, config]() {
				live_config_ = config;
				tls_config_ = &live_config_;
			});
		}
	}

	const Config& config() const {
		if(tls_config_) {
			return live_config_;
		} else {
			tls_config_ = &base_config_;
			return live_config_;
		}
	}
};

} // realm, ember