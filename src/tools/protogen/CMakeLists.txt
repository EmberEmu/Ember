# Copyright (c) 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME protogen)
set(LIBRARY_NAME    libprotogen)

set(EXECUTABLE_SRC
    main.cpp
)

set(LIBRARY_SRC
    Validator.cpp
    Validator.h
    Generator.cpp
    Generator.h
    Scope.h
    TypeUtils.h
    TypeUtils.cpp
)

set(PROTOGEN_TEMPLATES
    ${CMAKE_CURRENT_SOURCE_DIR}/templates/Message.h_
    ${CMAKE_CURRENT_SOURCE_DIR}/templates/Packets.h_
    ${CMAKE_CURRENT_SOURCE_DIR}/templates/Enum.h_
    ${CMAKE_CURRENT_SOURCE_DIR}/templates/Struct.h_
)

add_library(${LIBRARY_NAME} ${LIBRARY_SRC})
target_link_libraries(${LIBRARY_NAME} jsoncons::jsoncons)

add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_SRC})
target_link_libraries(${EXECUTABLE_NAME} ${LIBRARY_NAME} logger jsoncons::jsoncons)

if(MSVC)
    # jsoncons + jsonschema + inja + logger in one TU exceeds the default
    # section count in Debug builds.
    target_compile_options(${EXECUTABLE_NAME} PRIVATE /bigobj)
    target_compile_options(${LIBRARY_NAME}    PRIVATE /bigobj)
endif()

set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    PROTOGEN_TEMPLATES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/templates"
)

INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/tools)
install(FILES ${PROTOGEN_TEMPLATES} DESTINATION ${CMAKE_INSTALL_PREFIX}/tools/templates)
install(FILES ${PROTOGEN_TEMPLATES} DESTINATION ${CMAKE_INSTALL_PREFIX}/test_data/templates)
set_target_properties(${EXECUTABLE_NAME} PROPERTIES FOLDER "Tools")
set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Tools//Libraries")