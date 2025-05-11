# Copyright (c) 2016 - 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(FB_SCHEMA_TARGET_NAME FB_SCHEMA_COMPILE)

set(SERVICE_SCHEMAS
    ${CMAKE_CURRENT_SOURCE_DIR}/spark/services/Account.fbs
    ${CMAKE_CURRENT_SOURCE_DIR}/spark/services/Character.fbs
    ${CMAKE_CURRENT_SOURCE_DIR}/spark/services/Realm.fbs
    ${CMAKE_CURRENT_SOURCE_DIR}/spark/services/Discovery.fbs
)

set(FB_SCHEMAS
    ${SERVICE_SCHEMAS}
    ${CMAKE_CURRENT_SOURCE_DIR}/spark/Spark.fbs
    ${CMAKE_CURRENT_SOURCE_DIR}/packetlog/PacketLog.fbs
)

set(FLATC_ARGS "--cpp" "--gen-mutable" "--scoped-enums" "--bfbs-comments" "--gen-object-api")

flatbuffers_generate_headers(
  TARGET              ${FB_SCHEMA_TARGET_NAME}
  SCHEMAS             ${FB_SCHEMAS}
  INCLUDE             ${CMAKE_CURRENT_SOURCE_DIR}
  BINARY_SCHEMAS_DIR  ${CMAKE_BINARY_DIR}
  FLAGS               ${FLATC_ARGS}
)

target_include_directories(${FB_SCHEMA_TARGET_NAME} INTERFACE ${CMAKE_BINARY_DIR}/schemas/FB_SCHEMA_COMPILE)
set(TEMPLATE_DIR ${CMAKE_SOURCE_DIR}/src/tools/rpcgen/templates/)
build_spark_services("${SERVICE_SCHEMAS}" ${TEMPLATE_DIR} ${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR})