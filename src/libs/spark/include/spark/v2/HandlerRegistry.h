/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace ember::spark::v2 {

class Handler;

class HandlerRegistry final {
	// todo, merge these and remove duplicated code
	std::unordered_map<std::string, std::vector<Handler*>> services_;
	std::unordered_map<std::string, std::vector<Handler*>> clients_;
	mutable std::mutex mutex_;

public:
	void register_service(Handler* service);
	void register_client(Handler* client);
	void deregister_service(Handler* service);
	void deregister_client(Handler* service);

	std::vector<Handler*> services(const std::string& name) const;
	std::vector<Handler*> clients(const std::string& name) const;
	std::vector<std::string> services() const;
	std::vector<std::string> clients() const;
};

} // v2, spark, ember