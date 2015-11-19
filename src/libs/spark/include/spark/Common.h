/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <functional>
#include <spark/temp/MessageRoot_generated.h>
#include <boost/optional.hpp>

namespace ember { namespace spark {

struct Link;
enum class LinkState;

//namespace messaging { struct MessageRoot; }

typedef std::function<void(boost::optional<const messaging::MessageRoot*>)> TrackingHandler;
typedef std::function<void(const Link& link, const messaging::MessageRoot*)> MsgHandler;
typedef std::function<void(const Link& link, LinkState state)> LinkStateHandler;

}} // spark, ember