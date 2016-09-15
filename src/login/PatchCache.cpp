/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "PatchCache.h"
#include <shared/util/FileMD5.h>
#include <logger/Logging.h>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <botan/md5.h>

namespace bfs = boost::filesystem;

namespace ember { namespace patch_cache {


}} // patch_cache, ember