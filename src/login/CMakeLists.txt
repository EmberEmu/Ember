# Copyright (c) 2014 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME login)
set(LIBRARY_NAME    login_shared) # Shared library
set(LIBRARY_OBJECTS liblogin)     # Object library

set(GRUNT_SRC
    grunt/Magic.h
    grunt/Exceptions.h
    grunt/ResultCodes.h
    grunt/Handler.h
    grunt/Packets.h
    grunt/Packet.h
    grunt/PacketFwd.h
    grunt/Opcodes.h
    grunt/client/LoginChallenge.h
    grunt/client/LoginProof.h
    grunt/client/ReconnectProof.h
    grunt/client/RequestRealmList.h
    grunt/client/SurveyResult.h
    grunt/client/TransferAccept.h
    grunt/client/TransferResume.h

    grunt/server/LoginChallenge.h
    grunt/server/ReconnectChallenge.h
    grunt/server/LoginProof.h
    grunt/server/ReconnectProof.h
    grunt/server/RealmList.h
    grunt/server/TransferData.h
    grunt/server/TransferInitiate.h
)

set(LIBRARY_HDR
    Actions.h
    SessionBuilders.h
    LoginSession.h
    NetworkListener.h
    NetworkSession.h
    LoginHandler.h
    LoginHandlerBuilder.h
    SessionManager.h
    Authenticator.h
    RealmList.h
    Actions.h
    Patcher.h
    FilterTypes.h
    MonitorCallbacks.h
    AccountClient.h
    RealmClient.h
    PINAuthenticator.h
    ExecutablesChecksum.h
    PatchGraph.h
    IntegrityData.h
    IntegrityPlatforms.h
    LocaleMap.h
    LoginState.h
    Survey.h
    LoginHandlerFwd.h
    SocketType.h
    Service.h
    LoggingCallbacks.h
)

set(LIBRARY_SRC
    LoginHandler.cpp
    SessionManager.cpp
    LoginSession.cpp
    RealmList.cpp
    Authenticator.cpp
    MonitorCallbacks.cpp
    AccountClient.cpp
    RealmClient.cpp
    ExecutablesChecksum.cpp
    LocaleMap.cpp
    Survey.cpp
    Service.cpp
	Patcher.cpp
    PINAuthenticator.cpp
    IntegrityData.cpp
    PatchGraph.cpp
    grunt/Handler.cpp
)

if(WIN32)
    set(ICON_RESOURCE resources.rc)
endif()

set(EXECUTABLE_SOURCE
    main.cpp
    commands/Shutdown.h
    commands/Shutdown.cpp
	${ICON_RESOURCE}
)

source_group("Grunt Protocol" FILES ${GRUNT_SRC})

add_library(${LIBRARY_OBJECTS} OBJECT
	${LIBRARY_SRC}
	${LIBRARY_HDR}
    ${GRUNT_SRC}
)

target_include_directories(${LIBRARY_OBJECTS} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${LIBRARY_OBJECTS} PUBLIC ${ZLIB_LIBRARY} botan::botan Threads::Threads spark ports dbcreader stun service spark srp6 shared)

add_library(${LIBRARY_NAME} SHARED
	$<TARGET_OBJECTS:${LIBRARY_OBJECTS}>
)

add_dependencies(${LIBRARY_NAME}
    FB_SCHEMA_COMPILE
    SPARK_RPC_GENERATE
)

target_compile_definitions(${LIBRARY_OBJECTS} PUBLIC EXPORT_SERVICE)
target_link_libraries(${LIBRARY_NAME} PRIVATE ${LIBRARY_OBJECTS} INTERFACE service logger commands shared ${Boost_LIBRARIES})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(${LIBRARY_NAME} PROPERTIES OUTPUT_NAME ${LIBRARY_OBJECTS})

add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_SOURCE})
target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${LIBRARY_NAME} banner logger commands shared ${Boost_LIBRARIES} Threads::Threads)

install(TARGETS ${EXECUTABLE_NAME} ${LIBRARY_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
set_target_properties(${EXECUTABLE_NAME} PROPERTIES FOLDER "Services/Applications")
set_target_properties(${LIBRARY_OBJECTS} PROPERTIES FOLDER "Services/Libraries")
set_target_properties(${LIBRARY_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Shared Service Libraries")