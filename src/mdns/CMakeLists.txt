# Copyright (c) 2021 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME mdns)
set(LIBRARY_NAME    mdns_shared) # Shared library
set(LIBRARY_OBJECTS libmdns)     # Object library

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

add_library(${LIBRARY_OBJECTS} OBJECT
    ${LIBRARY_HDR}
	${LIBRARY_SRC}
)

add_library(${LIBRARY_NAME} SHARED 
    $<TARGET_OBJECTS:${LIBRARY_OBJECTS}>
)

target_include_directories(${LIBRARY_OBJECTS} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${LIBRARY_OBJECTS} PUBLIC service shared spark commands)

add_dependencies(${LIBRARY_NAME}
    FB_SCHEMA_COMPILE
    SPARK_RPC_GENERATE
)

set_property(TARGET ${LIBRARY_OBJECTS} PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(${LIBRARY_OBJECTS} PUBLIC EXPORT_SERVICE)
target_link_libraries(${LIBRARY_NAME} PRIVATE ${LIBRARY_OBJECTS} INTERFACE commands logger ${Boost_LIBRARIES} PUBLIC Threads::Threads)

if(WIN32)
    set(ICON_RESOURCE resources.rc)
endif()

add_executable(${EXECUTABLE_NAME}
    main.cpp
    ${ICON_RESOURCE}
)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${LIBRARY_NAME} service banner logger commands shared ${Boost_LIBRARIES} Threads::Threads)

install(TARGETS ${EXECUTABLE_NAME} ${LIBRARY_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
set_target_properties(${EXECUTABLE_NAME} PROPERTIES FOLDER "Services/Applications")
set_target_properties(${LIBRARY_OBJECTS} PROPERTIES FOLDER "Services/Libraries")
set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Shared Service Libraries")