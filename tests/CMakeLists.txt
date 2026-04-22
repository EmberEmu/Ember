# Copyright (c) 2014 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME unit_tests)

set(EXECUTABLE_SRC
    SRP6.cpp
    DynamicBuffer.cpp
    IntrusiveStorage.cpp
    BinaryStreamPMR.cpp
    GruntProtocol.cpp
    Patcher.cpp
    IPBan.cpp
    DNS.cpp
    PINAuthenticator.cpp
    UTF8Utilities.cpp
    STUN.cpp
    FNV1.cpp
    Ports.cpp
    GameIntegrity.cpp
    IntegrityData.cpp
    PatchGraph.cpp
    MPQ.cpp
    BinaryStream.cpp
    BufferAdaptor.cpp
    ThreadUtility.cpp
    BufferAdaptorPMR.cpp
    BufferUtility.cpp
    TLSBlockAllocator.cpp
    StaticBuffer.cpp
    GameVersion.cpp
    FileBuffer.cpp
    NullBuffer.cpp
    Utilities.cpp
    Commands.cpp
    ClientIdent.cpp
    PacketCrypto.cpp
	Protogen.cpp
)

if(WIN32)
    set(EXECUTABLE_SRC ${EXECUTABLE_SRC} resources.rc)
endif()

add_executable(${EXECUTABLE_NAME}
    ${EXECUTABLE_SRC}
    $<TARGET_OBJECTS:liblogin>
    $<TARGET_OBJECTS:libmdns>
)

target_include_directories(${EXECUTABLE_NAME} PUBLIC ../src ../src/tools)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE gtest gtest_main libprotogen liblogin libmdns ports shared allocators spark srp6 libmdns stun mpq ${BOTAN_TARGET} ${Boost_LIBRARIES})
gtest_discover_tests(${EXECUTABLE_NAME})

add_custom_command(TARGET ${EXECUTABLE_NAME} PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/tests/test_data ${CMAKE_CURRENT_BINARY_DIR}/test_data)

file(GLOB PROTOGEN_TEMPLATE_SRCS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/tools/protogen/templates/*.h_)

set(PROTOGEN_TEMPLATE_COPIES)

foreach(src IN LISTS PROTOGEN_TEMPLATE_SRCS)
    get_filename_component(name ${src} NAME)
    set(dst ${CMAKE_CURRENT_BINARY_DIR}/test_data/templates/${name})
    add_custom_command(
        OUTPUT ${dst}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${src} ${dst}
        DEPENDS ${src}
        COMMENT "Copying protogen template ${name}"
    )
    list(APPEND PROTOGEN_TEMPLATE_COPIES ${dst})
endforeach()

add_custom_target(copy_protogen_test_templates DEPENDS ${PROTOGEN_TEMPLATE_COPIES})
add_dependencies(${EXECUTABLE_NAME} copy_protogen_test_templates)

install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
install(DIRECTORY test_data/ DESTINATION ${CMAKE_INSTALL_PREFIX}/test_data)