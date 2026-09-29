# Copyright (c) 2018 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

file(GLOB CONFIG_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.conf.dist")

foreach(CONFIG_FILE IN LISTS CONFIG_FILES)
    get_filename_component(CONFIG_NAME "${CONFIG_FILE}" NAME)

    configure_file(
        "${CONFIG_FILE}"
        "${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_NAME}"
        @ONLY
    )

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_NAME}" DESTINATION ${CMAKE_INSTALL_PREFIX}/configs
    )
endforeach()