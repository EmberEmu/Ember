# Copyright (c) 2014 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME dbcparser)

set(EXECUTABLE_SRC
	main.cpp
	Parser.h
	Parser.cpp
	Exception.h
	Validator.h
	Validator.cpp
	Generator.h
	Generator.cpp
	TypeUtils.cpp
	TypeUtils.h
	Types.h
	TreeNode.h
	DBCGenerator.h
	DBCGenerator.cpp
	SQLDDLGenerator.h
	SQLDDLGenerator.cpp
	SQLDMLGenerator.h
	SQLDMLGenerator.cpp
	DBCHeader.h
)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/deps/rapidxml)
add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_SRC})
target_link_libraries(${EXECUTABLE_NAME} INTERFACE shared PRIVATE bprinter logger spark ${Boost_LIBRARIES} Threads::Threads)
install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/tools)
set_target_properties(${EXECUTABLE_NAME} PROPERTIES FOLDER "Tools")