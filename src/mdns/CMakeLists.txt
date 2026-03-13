# Copyright (c) 2021 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME mdns)
set(LIBRARY_NAME    libmdns)

set(LIBRARY_HDR
    ServiceRegistry.h
    MulticastSocket.h
    Server.h
    Socket.h
    DNSDefines.h
    Handler.h
    Utility.h
    Serialisation.h
    detail/Parser.h
    NSDService.h
    Service.h
    StreamType.h
    ServiceContext.h
    ServiceContextImpl.h
)

set(LIBRARY_SRC
    ServiceRegistry.cpp
    MulticastSocket.cpp
    Server.cpp
    Utility.cpp
    Serialisation.cpp
    detail/Parser.cpp
    NSDService.cpp
    Service.cpp
    ServiceContext.cpp
)

add_library(${LIBRARY_NAME} ${LIBRARY_HDR} ${LIBRARY_SRC})
add_dependencies(${LIBRARY_NAME} SPARK_RPC_GENERATE)
target_link_libraries(${LIBRARY_NAME} PRIVATE spark PUBLIC logger commands shared ${Boost_LIBRARIES} Threads::Threads)

if(WIN32)
    set(ICON_RESOURCE resources.rc)
endif()

add_executable(${EXECUTABLE_NAME}
    main.cpp
    ${ICON_RESOURCE}
)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${LIBRARY_NAME} banner logger commands shared ${Boost_LIBRARIES} Threads::Threads)

install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
set_target_properties(mdns PROPERTIES FOLDER "Services/Applications")
set_target_properties(libmdns PROPERTIES FOLDER "Services/Libraries")