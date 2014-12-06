# Copyright (c) 2014 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME dbc-parser)
set(EXECUTABLE_SRC
	main.cpp
	Parser.h
	Parser.cpp
	Field.h
	Exception.h
	Definition.h
	Validator.h
	Validator.cpp
	Generator.h
	Generator.cpp
	Keywords.h
	bprinter/table_printer.cpp
	bprinter/table_printer.h
	bprinter/impl/table_printer.tpp.h
	)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/deps/rapidxml)
add_executable(dbc-parser ${EXECUTABLE_SRC} ${version_file})
target_link_libraries(${EXECUTABLE_NAME} ${Boost_LIBRARIES})
target_link_libraries(${EXECUTABLE_NAME} ${BOTAN_LIBRARY})