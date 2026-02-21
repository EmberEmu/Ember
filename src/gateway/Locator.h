/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once 

#include <shared/game/GameVersion.h>
#include <span>

namespace ember::gateway {

class EventDispatcher;
class CharacterClient;
class AccountClient;
class RealmService;
class RealmQueue;
struct Config;

class Locator {
	inline static EventDispatcher* dispatcher_;
	inline static CharacterClient* character_;
	inline static AccountClient* account_;
	inline static RealmService* realm_;
	inline static RealmQueue* queue_;
	inline static Config* config_;
	inline static std::span<const GameVersion> builds_;

public:
	static void set(Config* config) { config_ = config; }
	static void set(RealmQueue* queue) { queue_ = queue; }
	static void set(RealmService* realm) { realm_ = realm; }
	static void set(AccountClient* account) { account_ = account; }
	static void set(CharacterClient* character) { character_ = character; }
	static void set(EventDispatcher* dispatcher) { dispatcher_ = dispatcher; }
	static void set(std::span<const GameVersion> builds) { builds_ = builds; }

	static Config* config() { return config_; }
	static RealmQueue* queue() { return queue_; }
	static RealmService* realm() { return realm_; }
	static AccountClient* account() { return account_; }
	static CharacterClient* character() { return character_; }
	static EventDispatcher* dispatcher() { return dispatcher_; }
	static std::span<const GameVersion> builds() { return builds_; }
};

} // gateway, ember