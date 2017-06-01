# Copyright (c) 2015, 2016 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME spark)

add_library(${LIBRARY_NAME}
            src/Service.cpp
            src/SessionManager.cpp
            src/Listener.cpp
            src/MessageHandler.cpp
            src/HeartbeatService.cpp
            src/EventDispatcher.cpp
            src/Utility.cpp
            src/TrackingService.cpp
            src/ServicesMap.cpp
            src/ServiceDiscovery.cpp
            src/ServiceListener.cpp
            include/spark/EventHandler.h
            include/spark/ServiceListener.h
            include/spark/ServiceDiscovery.h
            include/spark/ServicesMap.h
            include/spark/Common.h
            include/spark/TrackingService.h
            include/spark/Link.h
            include/spark/EventDispatcher.h
            include/spark/HeartbeatService.h
            include/spark/MessageHandler.h
            include/spark/Buffer.h
            include/spark/buffers/ChainedBuffer.h
            include/spark/buffers/ChainedNode.h
            include/spark/buffers/BufferSequence.h
            include/spark/BinaryStream.h
            include/spark/SafeBinaryStream.h
            include/spark/Spark.h
            include/spark/Service.h
            include/spark/Listener.h
            include/spark/NetworkSession.h
            include/spark/SessionManager.h
            include/spark/Utility.h
            include/spark/Exception.h
            include/spark/Helpers.h
)

target_link_libraries(${LIBRARY_NAME} shared ${Boost_LIBRARIES})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)