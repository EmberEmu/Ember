# Copyright (c) 2015 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME spark)

add_library(${LIBRARY_NAME}
            src/Service.cpp
            src/SessionManager.cpp
            src/Listener.cpp
            src/Client.cpp
            src/Router.cpp
            src/MessageHandler.cpp
            include/spark/MessageHandler.h
            include/spark/Buffer.h
            include/spark/buffers/ChainedBuffer.h
            include/spark/buffers/ChainedNode.h
            include/spark/BinaryStream.h
            include/spark/Spark.h
            include/spark/Service.h
            include/spark/Listener.h
            include/spark/Client.h
            include/spark/Router.h
            include/spark/NetworkSession.h
            include/spark/SessionManager.h
)

target_link_libraries(${LIBRARY_NAME} shared ${Boost_LIBRARIES})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)