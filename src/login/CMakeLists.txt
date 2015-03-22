# Copyright (c) 2014 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME login)

set(EXECUTABLE_HDR
    ../shared/Banner.h
    ../shared/TCPServer.h
    ../shared/memory/ASIOAllocator.h
    ../shared/database/daos/UserDAO.h
    ../shared/database/daos/IPBanDAO.h
    ../shared/IPBanCache.h
    ../shared/misc/PacketStream.h
    ../shared/threading/ThreadPool.h
    LoginHandlerBuilder.h
    LoginManager.h
    LoginHandler.h
    PacketBuffer.h
    Session.h
    Protocol.h
    GameVersion.h
    Authenticator.h
    Actions.h
    Patcher.h
    )

set(EXECUTABLE_SRC
    ../shared/threading/ThreadPool.cpp
    ../shared/Banner.cpp
    LoginHandler.cpp
    LoginManager.cpp
    Authenticator.cpp
    Patcher.cpp
    main.cpp
    )

include_directories(${CMAKE_SOURCE_DIR}/src)
add_executable(login ${EXECUTABLE_HDR} ${EXECUTABLE_SRC} ${version_file})
target_link_libraries(${EXECUTABLE_NAME} ${Boost_LIBRARIES})
target_link_libraries(${EXECUTABLE_NAME} ${BOTAN_LIBRARY})
target_link_libraries(${EXECUTABLE_NAME} ${MYSQLCCPP_LIBRARY})
target_link_libraries(${EXECUTABLE_NAME} logging)
target_link_libraries(${EXECUTABLE_NAME} srp6)
target_link_libraries(${EXECUTABLE_NAME} conpool)
INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/configs/login/ DESTINATION ${CMAKE_INSTALL_PREFIX})