# Copyright (c) 2016 - 2021 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(FB_SCHEMA_TARGET_NAME FB_SCHEMA_COMPILE)

set(FB_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/spark/Services.fbs
    ${CMAKE_CURRENT_SOURCE_DIR}/spark/Account.fbs
    ${CMAKE_CURRENT_SOURCE_DIR}/spark/Character.fbs
    ${CMAKE_CURRENT_SOURCE_DIR}/spark/Core.fbs
    ${CMAKE_CURRENT_SOURCE_DIR}/spark/RealmStatus.fbs
    ${CMAKE_CURRENT_SOURCE_DIR}/spark/Multicast.fbs

    ${CMAKE_CURRENT_SOURCE_DIR}/spark/v2/Spark.fbs
    ${CMAKE_CURRENT_SOURCE_DIR}/spark/v2/Test.fbs

    ${CMAKE_CURRENT_SOURCE_DIR}/packetlog/PacketLog.fbs
)

set(FLATC_ARGS "--gen-mutable" "--scoped-enums" "--schema" "-b" "--bfbs-comments")
build_flatbuffers("${FB_HEADERS}" ${CMAKE_CURRENT_SOURCE_DIR} ${FB_SCHEMA_TARGET_NAME} "" ${CMAKE_BINARY_DIR} "" "")

set(SPARKC_BFBS_DIRS     ${CMAKE_BINARY_DIR})
set(SPARKC_TARGET_NAME   SPARKC_SERVICE_COMPILE)
set(CODE_OUTPUT_DIR      ${CMAKE_BINARY_DIR})
set(FILE_LOG_VERBOSITY   info)

build_spark_services(${SPARKC_BFBS_DIRS} ${CODE_OUTPUT_DIR} ${SPARKC_TARGET_NAME} ${FILE_LOG_VERBOSITY})