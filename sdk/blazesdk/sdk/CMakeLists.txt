# Copyright (c) 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME blazesdk)

set(LIBRARY_SRC
    src/dummy.h
	src/dummy.c
)

add_library(${LIBRARY_NAME} ${LIBRARY_SRC})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(${EXECUTABLE_NAME} PROPERTIES FOLDER "SDKs")