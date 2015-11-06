# Copyright (c) 2015 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_library(spark
            include/spark/Buffer.h
            include/spark/BufferChain.h
            include/spark/BufferChainNode.h
            include/spark/BinaryStream.h
            dummy.cpp
)

target_include_directories(spark PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(spark PROPERTIES LINKER_LANGUAGE CXX)