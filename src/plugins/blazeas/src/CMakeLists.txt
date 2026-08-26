# Copyright (c) 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME blazeas)

set(LIBRARY_HDR
    BlazeAS.h
    Visibility.h
)

set(LIBRARY_SRC
    main.cpp
    BlazeAS.cpp
)

add_library(${LIBRARY_NAME} SHARED ${LIBRARY_SRC})
target_link_libraries(${LIBRARY_NAME} PRIVATE Threads::Threads Angelscript::angelscript blazesdk)
install(TARGETS ${LIBRARY_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/tools/plugins)
set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Plugins")