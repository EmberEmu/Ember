# Copyright (c) 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME mpq)

set(ARCHIVE
	include/mpq/base/Archive.h
	include/mpq/base/FileArchive.h
	include/mpq/base/MemoryArchive.h
	include/mpq/base/MappedArchive.h
	src/base/MemoryArchive.cpp
)

set(ARCHIVE_V0
	include/mpq/v0/FileArchive.h
	include/mpq/v0/MemoryArchive.h
	include/mpq/v0/MappedArchive.h
	src/v0/FileArchive.cpp
	src/v0/MemoryArchive.cpp
)

source_group("Archive" FILES ${ARCHIVE})
source_group("Archive\\v0" FILES ${ARCHIVE_V0})

add_library(${LIBRARY_NAME}
	${ARCHIVE}
	${ARCHIVE_V0}
	include/mpq/MPQ.h
	include/mpq/Archive.h
	include/mpq/Structures.h
	include/mpq/Crypt.h
	include/mpq/Compression.h
	include/mpq/Exception.h
	include/mpq/SharedDefs.h
	include/mpq/ExtractionSink.h
	include/mpq/FileSink.h
	include/mpq/MemorySink.h
	include/mpq/Utility.h
	src/MPQ.cpp
	src/Compression.cpp
	src/PKCompression.cpp
	src/Utility.cpp
)

target_link_libraries(${LIBRARY_NAME} shared spark storm_pklib
                      bzip2 lzma storm_adpcm storm_sparse storm_huffman
                      ${Boost_LIBRARIES} ${ZLIB_LIBRARY})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(mpq PROPERTIES FOLDER "Libraries")
