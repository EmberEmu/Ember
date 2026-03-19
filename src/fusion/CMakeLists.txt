# Copyright (c) 2024 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME fusion)

if(WIN32)
    set(ICON_RESOURCE resources.rc)
endif()

set(SOURCE_LIST
    main.cpp
    CommandHelpers.h
    CommandHelpers.cpp
    Prototypes.h
    Library.h
    ServiceRunner.h
    ServiceRunner.cpp
    Wrapped.h
)

if(WIN32)
    set(SOURCE_LIST ${SOURCE_LIST} win32/Library.cpp)
else()
    set(SOURCE_LIST ${SOURCE_LIST} posix/Library.cpp)
endif()

add_executable(${EXECUTABLE_NAME}
    ${SOURCE_LIST}
    ${ICON_RESOURCE}
)

set(SERVICE_LIBRARIES
    login_shared
    mdns_shared
    libworld
    librealm
    libaccount
    libcharacter
)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${SERVICE_LIBRARIES} banner logger commands shared ${Boost_LIBRARIES})
target_include_directories(${EXECUTABLE_NAME} PRIVATE ../)

install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
set_target_properties(fusion PROPERTIES FOLDER "Services/Applications")