# Copyright (c) 2021 - 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME rpcgen)

set(EXECUTABLE_SRC
    main.cpp
    Printer.h
    Printer.cpp
    SchemaParser.h
    SchemaParser.cpp
	Utility.h
	Utility.cpp
    )

include_directories(${CMAKE_SOURCE_DIR}/deps/nlohmann)
add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_SRC})
target_link_libraries(${EXECUTABLE_NAME} logging shared ${Boost_LIBRARIES} ${flatbuffers})
INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/tools)
set_target_properties(${EXECUTABLE_NAME} PROPERTIES FOLDER "Tools")