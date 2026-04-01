/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace ember {

enum FilterType {
	lf_uncategorised =  1, // do not use - reserved for the logger implementation
	lf_monitoring    =  2,
	lf_db_conn_pool  =  4,
	lf_network       =  8,
	lf_spark         = 16,
	lf_naughty_user  = 32,
	lf_reserved_1    = 64
	// all higher valus are reserved for services
};

} // ember