# Copyright (c) 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME service)

add_library(${LIBRARY_NAME} INTERFACE
    include/service/Config.h
	include/service/Service.h
)

target_include_directories(${LIBRARY_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Common Libraries")