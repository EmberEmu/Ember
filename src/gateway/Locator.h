/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once 

namespace ember {

class CharacterService;
class AccountService;
class RealmService;
class RealmQueue;
//struct Config;

class Locator {
	static CharacterService* character_;
	static AccountService* account_;
	static RealmService* realm_;
	static RealmQueue* queue_;
	//static Config* config_;

public:
	//static void set(Config* config) { config_ = config; }
	static void set(RealmQueue* queue) { queue_ = queue; }
	static void set(RealmService* realm) { realm_ = realm; }
	static void set(AccountService* account) { account_ = account; }
	static void set(CharacterService* character) { character_ = character; }

	//static Config* config() { return config_; }
	static RealmQueue* queue() { return queue_; }
	static RealmService* realm() { return realm_; }
	static AccountService* account() { return account_; }
	static CharacterService* character() { return character_; }
};

} // ember