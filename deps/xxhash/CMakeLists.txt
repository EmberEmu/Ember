# Copyright (c) 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME xxhash)

add_library(${LIBRARY_NAME} STATIC
    xxh_x86dispatch.c
    xxh_x86dispatch.h
    xxh3.h
    xxhash.c
    xxhash.h
)

set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Dependencies")