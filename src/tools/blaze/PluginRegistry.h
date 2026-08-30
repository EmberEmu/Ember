/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Common.h"
#include "Plugin.h"
#include <boost/unordered/unordered_flat_map.hpp>
#include <memory>
#include <mutex>
#include <cstddef>

namespace ember::blaze {

class PluginRegistry final {
	boost::unordered_flat_map<PluginID, std::shared_ptr<Plugin>> plugins_;
	mutable std::mutex lock_;

public:
	bool add(Plugin plugin);
	bool remove(PluginID pid);
	std::shared_ptr<Plugin> locate(PluginID pid) const;
	std::size_t size() const;
};

} // blaze, ember