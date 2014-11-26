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

namespace SRP6 {

class Server {
	const Botan::BigInt v, N, b, k{ 3 };
	Botan::BigInt B, A;

public:
	Server(const Generator& g, const Botan::BigInt& v, int key_size = 32);
	inline const Botan::BigInt& public_ephemeral() const { return B; }
	SessionKey session_key(const Botan::BigInt& A);
	Botan::BigInt generate_proof(const SessionKey& key, const Botan::BigInt& client_proof) const;
};

} //SRP6