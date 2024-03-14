# Copyright (c) 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME mpq)

add_library(${LIBRARY_NAME}
           include/mpq/mpq.h
		   include/mpq/Archive.h
		   include/mpq/base/Archive.h
		   include/mpq/base/FileArchive.h
		   include/mpq/base/MemoryArchive.h
		   include/mpq/base/MappedArchive.h
		   include/mpq/v0/FileArchive.h
		   include/mpq/v0/MemoryArchive.h
		   include/mpq/v0/MappedArchive.h
		   include/mpq/Structures.h
		   src/Archive.cpp
		   src/MPQ.cpp
           )

target_link_libraries(${LIBRARY_NAME} shared spark ${Boost_LIBRARIES})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(mpq PROPERTIES FOLDER "Libraries")
