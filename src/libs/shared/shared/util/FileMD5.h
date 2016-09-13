/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <string>

namespace Botan { class BigInt; }

namespace ember { namespace util {

Botan::BigInt generate_md5(const std::string& file);

}} // util, ember