# Copyright (c) 2016 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME world)
set(LIBRARY_NAME libworld)

set(LIBRARY_HDR
    Service.h
    MapRunner.h
    Watchdog.h
    utilities/DeltaTimer.h
    utilities/Utility.h
)

set(LIBRARY_SRC
    Service.cpp
    MapRunner.cpp
    Watchdog.cpp
    utilities/DeltaTimer.cpp
    utilities/Utility.cpp
)

add_library(${LIBRARY_NAME} ${LIBRARY_HDR} ${LIBRARY_SRC})
target_link_libraries(${LIBRARY_NAME} PRIVATE dbcreader spark Threads::Threads PUBLIC logger commands shared ${Boost_LIBRARIES})

if(WIN32)
    set(ICON_RESOURCE resources.rc)
endif()

add_executable(${EXECUTABLE_NAME}
    main.cpp
    ${ICON_RESOURCE}
)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${LIBRARY_NAME} banner logger commands shared ${Boost_LIBRARIES})

install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
set_target_properties(world PROPERTIES FOLDER "Services/Applications")
set_target_properties(libworld PROPERTIES FOLDER "Services/Libraries")