# Copyright (c) 2016 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME blaze)
set(LIBRARY_NAME    libblaze)

set(LIBRARY_HDR
    Blaze.h
    Client.h
    Connection.h
    Library.h
    Plugin.h
    PluginCommands.h
    Common.h
    PluginRegistry.h
    PluginState.h
    ServiceContext.h
    ServiceContextImpl.h
    commands/Commands.h
    commands/PluginControl.h
    api/Logging.h
    api/Commands.h
    api/State.h
)

set(LIBRARY_SRC
    Blaze.cpp
    Client.cpp
    Connection.cpp
    PluginCommands.cpp
    PluginRegistry.cpp
    ServiceContext.cpp
    commands/Commands.cpp
    commands/PluginControl.cpp
    api/Logging.cpp
    api/Commands.cpp
    api/State.cpp
)

if(WIN32)
    set(LIBRARY_SRC ${LIBRARY_SRC} win32/Library.cpp)
else()
    set(LIBRARY_SRC ${LIBRARY_SRC} posix/Library.cpp)
endif()

add_library(${LIBRARY_NAME} ${LIBRARY_HDR} ${LIBRARY_SRC})
target_link_libraries(${LIBRARY_NAME} PRIVATE shared commands logger ${Boost_LIBRARIES})

if(WIN32)
    set(ICON_RESOURCE resources.rc)
endif()

add_executable(${EXECUTABLE_NAME}
    main.cpp
    api/Logging.cpp
    ${ICON_RESOURCE}
)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${LIBRARY_NAME} banner logger shared commands ${Boost_LIBRARIES})
install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/tools)
set_target_properties(${EXECUTABLE_NAME} PROPERTIES FOLDER "Tools")
set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Tools//Libraries")