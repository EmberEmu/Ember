# Copyright (c) 2016 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME dbcreader)

add_library(${LIBRARY_NAME}
            src/Linker.cpp
            src/DiskLoader.cpp
            include/dbcreader/DBCReader.h
            include/dbcreader/DBCHeader.h
            include/dbcreader/DBCMap.h
            include/dbcreader/DiskLoader.h
            include/dbcreader/Linker.h
            include/dbcreader/Loader.h
            include/dbcreader/DiskDefs.h
            include/dbcreader/MemoryDefs.h
            include/dbcreader/Storage.h
           )

target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)