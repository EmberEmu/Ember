/*
 * Copyright (c) 2016 - 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once 

#include <span>

namespace ember::realm {

class EventDispatcher;
class CharacterClient;
class AccountClient;
class RealmService;
class RealmQueue;
class ConfigStore;

class Locator {
	inline static EventDispatcher* dispatcher_;
	inline static CharacterClient* character_;
	inline static AccountClient* account_;
	inline static RealmService* realm_;
	inline static RealmQueue* queue_;
	inline static ConfigStore* config_store_;

public:
	static void set(RealmQueue* queue) { queue_ = queue; }
	static void set(RealmService* realm) { realm_ = realm; }
	static void set(AccountClient* account) { account_ = account; }
	static void set(CharacterClient* character) { character_ = character; }
	static void set(EventDispatcher* dispatcher) { dispatcher_ = dispatcher; }
	static void set(ConfigStore* config_store) { config_store_ = config_store; }

	static RealmQueue* queue() { return queue_; }
	static RealmService* realm() { return realm_; }
	static AccountClient* account() { return account_; }
	static CharacterClient* character() { return character_; }
	static EventDispatcher* dispatcher() { return dispatcher_; }
	static ConfigStore* config_store() { return config_store_; }
};

} // realm, ember