# Copyright (c) 2016 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME blaze)
set(LIBRARY_NAME    libblaze)

set(LIBRARY_HDR
    Blaze.h
)

set(LIBRARY_SRC
    Blaze.cpp
)

add_library(${LIBRARY_NAME} ${LIBRARY_HDR} ${LIBRARY_SRC})
target_link_libraries(${LIBRARY_NAME} PRIVATE Angelscript::angelscript ${Boost_LIBRARIES})

add_executable(${EXECUTABLE_NAME}
    main.cpp
    ${ICON_RESOURCE}
)

if(WIN32)
    set(ICON_RESOURCE resources.rc)
endif()

target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${Boost_LIBRARIES})
install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/tools)
set_target_properties(blaze PROPERTIES FOLDER "Tools")