/*
 * Copyright (c) 2016 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "LocaleMap.h"

namespace ember {

std::unordered_map<grunt::Locale, dbc::Cfg_Categories::Region> locale_map {
	{ grunt::Locale::enGB, dbc::Cfg_Categories::Region::EUROPE },
	{ grunt::Locale::deDE, dbc::Cfg_Categories::Region::EUROPE },
	{ grunt::Locale::frFR, dbc::Cfg_Categories::Region::EUROPE },
	{ grunt::Locale::esES, dbc::Cfg_Categories::Region::EUROPE },
	{ grunt::Locale::ptBR, dbc::Cfg_Categories::Region::EUROPE },
	{ grunt::Locale::ptPT, dbc::Cfg_Categories::Region::EUROPE },
	{ grunt::Locale::ruRU, dbc::Cfg_Categories::Region::EUROPE },
	{ grunt::Locale::itIT, dbc::Cfg_Categories::Region::EUROPE },
	{ grunt::Locale::ptPT, dbc::Cfg_Categories::Region::EUROPE },
	{ grunt::Locale::enUS, dbc::Cfg_Categories::Region::UNITED_STATES },
	{ grunt::Locale::esMX, dbc::Cfg_Categories::Region::UNITED_STATES },
	{ grunt::Locale::enCN, dbc::Cfg_Categories::Region::CHINA },
	{ grunt::Locale::koKR, dbc::Cfg_Categories::Region::KOREA },
	{ grunt::Locale::enTW, dbc::Cfg_Categories::Region::TAIWAN },
	{ grunt::Locale::zhTW, dbc::Cfg_Categories::Region::TAIWAN },
};

};