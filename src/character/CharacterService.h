/*
 * Copyright (c) 2024 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <Characterv2ServiceStub.h>

namespace ember {

namespace em = messaging::Characterv2;

class CharacterService final : public services::Characterv2Service {
	log::Logger& logger_;

	void on_link_up(const spark::v2::Link& link) override;
	void on_link_down(const spark::v2::Link& link) override;

public:
	CharacterService(spark::v2::Server& spark, log::Logger& logger);
};

} // ember