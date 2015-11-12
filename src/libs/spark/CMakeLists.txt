# Copyright (c) 2015 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME spark)

add_library(${LIBRARY_NAME}
            src/Service.cpp
            src/Listener.cpp
            include/spark/Buffer.h
            include/spark/BufferChain.h
            include/spark/BufferChainNode.h
            include/spark/BinaryStream.h
            include/spark/Spark.h
            include/spark/Service.h
            include/spark/Listener.h
)

target_link_libraries(${LIBRARY_NAME} shared ${Boost_LIBRARIES})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)