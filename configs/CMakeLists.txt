# Copyright (c) 2018 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

FILE(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/*.conf.dist")
INSTALL(FILES ${files} DESTINATION ${CMAKE_INSTALL_PREFIX}/configs)