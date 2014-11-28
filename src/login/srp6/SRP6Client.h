/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "SRP6Util.h"
#include "SRP6Generator.h"
#include "SRP6Exception.h"
#include <botan/bigint.h>
#include <string>

namespace SRP6 {

class Client {
	const Generator gen_;
	const Botan::BigInt v_, a_;
	Botan::BigInt A_, B_, k_{ 3 };
	Botan::SecureVector<Botan::byte> salt_;
	std::string identifier_, password_;

public:
	Client(std::string identifier, std::string password, Generator gen, int key_size = 32,
	       bool srp6a = false);
	Client(std::string identifier, std::string password, Generator gen, Botan::BigInt a,
	       bool srp6a = false);
	SessionKey session_key(const Botan::BigInt& B, const Botan::SecureVector<Botan::byte>& salt,
	                       bool interleave = true);
	Botan::BigInt generate_proof(const SessionKey& key) const;
	inline const Botan::BigInt& public_ephemeral() const { return A_; }
};

} //SRP6