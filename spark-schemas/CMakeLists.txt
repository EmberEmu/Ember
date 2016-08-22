# Copyright (c) 2016 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(SPARK_TARGET_NAME SPARK_SCHEMA_COMPILE)

set(FB_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Account.fbs
    ${CMAKE_CURRENT_SOURCE_DIR}/Character.fbs
    ${CMAKE_CURRENT_SOURCE_DIR}/Core.fbs
    ${CMAKE_CURRENT_SOURCE_DIR}/RealmStatus.fbs
    ${CMAKE_CURRENT_SOURCE_DIR}/Multicast.fbs
)

build_flatbuffers("${FB_HEADERS}" ${CMAKE_CURRENT_SOURCE_DIR} ${SPARK_TARGET_NAME} "" ${CMAKE_BINARY_DIR} "" "")