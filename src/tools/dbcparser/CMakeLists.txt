# Copyright (c) 2014 - 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME dbc-parser)

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
	bprinter/table_printer.cpp
	bprinter/table_printer.h
	bprinter/impl/table_printer.tpp.h
	DBCHeader.h
	)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/deps/rapidxml)
add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_SRC} ${version_file})
target_link_libraries(${EXECUTABLE_NAME} logger spark shared ${Boost_LIBRARIES} Threads::Threads)

INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/tools)
set_target_properties(dbc-parser PROPERTIES FOLDER "Tools")