/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace ember::spark::v2 {

class RemotePeer;
class Handler;

class Peers final {
	std::unordered_map<std::string, std::shared_ptr<RemotePeer>> peers_;
	std::mutex lock_;

public:
	void add(std::string key, std::shared_ptr<RemotePeer> peer);
	void remove(const std::string& key);
	std::shared_ptr<RemotePeer> find(const std::string& key);
	void notify_remove_handler(Handler* handler);
};

} // spark, ember