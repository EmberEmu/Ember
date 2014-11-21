/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "SRP6Util.h"
#include <botan/bigint.h>
#include <string>
#include <stdexcept>

namespace SRP6 {

class exception : public std::runtime_error {
public:
	exception() : std::runtime_error("An unknown SRP6 exception occured!") { }
	exception(std::string msg) : std::runtime_error(msg) { };
};

class ClientSession {
public:
	bool verify_server_proof();
};

class ServerSession {
	const Botan::BigInt v_, N_, b_;
	const Botan::BigInt k_{3};
	Botan::BigInt B_, A_;

public:
	ServerSession(const Generator& g, const Botan::BigInt& v, const Botan::BigInt& N,
                  int key_size = 32);
	inline const Botan::BigInt& public_ephemeral() const { return B_; }
	SessionKey session_key(const Botan::BigInt& A);
	Botan::BigInt generate_proof(const SessionKey& key,
		const Botan::BigInt& client_proof) const;
};

} //SRP6