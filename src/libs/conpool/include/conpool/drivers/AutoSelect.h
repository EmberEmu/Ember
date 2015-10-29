/*
 * Copyright (c) 2015 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <shared/CompilerWarn.h>

#ifdef DB_MYSQL
	#include <conpool/drivers/MySQL/Driver.h>
	#include <conpool/drivers/MySQL/Config.h>
#elif DB_POSTGRESQL
	#include <conpool/drivers/PostgreSQL/Config.h>
#endif

namespace ember { namespace drivers {

#ifdef DB_MYSQL
	typedef MySQL DriverType;
#elif DB_POSTGRESQL
	typedef PostgreSQL DriverType;
#else
	#pragma message WARN("Cannot compile Ember without defining a DBMS!")
#endif

}} //drivers, ember
