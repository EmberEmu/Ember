/*
 * Copyright (c) 2015, 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Services_generated.h"
#include <vector>
#include <cstdint>

namespace ember::spark::inline v1::detail {

typedef std::underlying_type<messaging::Service>::type ServicesType;

// utility functions to workaround FlatBuffers' lack of proper C++11 support
std::vector<ServicesType> services_to_underlying(const std::vector<messaging::Service>& services);
std::vector<messaging::Service> underlying_to_services(const std::vector<ServicesType>& services);


} // detail, spark, ember