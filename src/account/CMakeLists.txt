# Copyright (c) 2015 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME account)
set(LIBRARY_NAME    libaccount)

set(LIBRARY_HDR
    FilterTypes.h
    AccountService.h
    Sessions.h
    AccountHandler.h
    Service.h
    LoggingCallbacks.h
    ServiceContext.h
    ServiceContextImpl.h
    InitHelpers.h
)

set(LIBRARY_SRC
    AccountService.cpp
    Sessions.cpp
    AccountHandler.cpp
    Service.cpp
    ServiceContext.cpp
)

include_directories(${SPARK_INCLUDES_DIR})

add_library(${LIBRARY_NAME} SHARED 
    ${LIBRARY_HDR}
    ${LIBRARY_SRC}
)

target_compile_definitions(${LIBRARY_NAME} PUBLIC EXPORT_SERVICE)
add_dependencies(${LIBRARY_NAME} FB_SCHEMA_COMPILE)
add_dependencies(${LIBRARY_NAME} SPARK_RPC_GENERATE)
target_link_libraries(${LIBRARY_NAME} PRIVATE spark conpool ${MYSQLCCPP_LIBRARY} Threads::Threads PUBLIC service thread logger commands shared ${Boost_LIBRARIES})

if(WIN32)
    set(ICON_RESOURCE resources.rc)
endif()

add_executable(${EXECUTABLE_NAME}
    main.cpp
    ${ICON_RESOURCE}
)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${LIBRARY_NAME} banner logger commands shared ${Boost_LIBRARIES} Threads::Threads)

install(TARGETS ${EXECUTABLE_NAME} ${LIBRARY_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
set_target_properties(${EXECUTABLE_NAME} PROPERTIES FOLDER "Services/Applications")
set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Services/Libraries")
