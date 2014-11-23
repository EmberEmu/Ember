# Copyright (c) 2014 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME login)
set(EXECUTABLE_SRC
	../shared/Banner.cpp
	srp6/SRP6.h
	srp6/SRP6.cpp
	srp6/SRP6Util.h
	srp6/SRP6Util.cpp
	srp6/SRP6Generator.h
	main.cpp
	)

include_directories(${CMAKE_SOURCE_DIR}/src/)
add_executable(login ${EXECUTABLE_SRC} ${version_file})
target_link_libraries(${EXECUTABLE_NAME} ${Boost_LIBRARIES})
target_link_libraries(${EXECUTABLE_NAME} ${BOTAN_LIBRARY})