/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "PluginRegistry.h"

namespace ember::blaze {

bool PluginRegistry::add(Plugin plugin) {
	std::lock_guard lg(lock_);

	const auto pid = plugin.pid();
	auto ptr = std::make_shared<Plugin>(std::move(plugin));
	return plugins_.try_emplace(pid, std::move(ptr)).second;
}

bool PluginRegistry::remove(PluginID pid) {
	std::lock_guard lg(lock_);
	return !!plugins_.erase(pid);
}

std::shared_ptr<Plugin> PluginRegistry::locate(PluginID pid) const {
	std::lock_guard lg(lock_);

	if(auto plugin = plugins_.find(pid); plugin != plugins_.end()) {
		return plugin->second;
	}

	return nullptr;
}

std::size_t PluginRegistry::size() const {
	std::lock_guard lg(lock_);
	return plugins_.size();
}

// todo, temporary, hopefully
std::vector<std::shared_ptr<Plugin>> PluginRegistry::all() const {
	std::lock_guard lg(lock_);
	std::vector<std::shared_ptr<Plugin>> plugins;

	for(auto& plugin : plugins_ | std::views::values) {
		plugins.emplace_back(plugin);
	}

	return plugins;
}

} // blaze, ember