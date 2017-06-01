/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <logger/Logging.h>
#include <boost/program_options.hpp>
#include <memory>

namespace ember::util {

std::unique_ptr<ember::log::Logger> init_logging(const boost::program_options::variables_map& args);

} // util, ember