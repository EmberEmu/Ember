# Copyright (c) 2015 - 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME spark)

set(CORE
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
    include/spark/Spark.h
    include/spark/Service.h
    include/spark/Listener.h
    include/spark/NetworkSession.h
    include/spark/SessionManager.h
    include/spark/Utility.h
    include/spark/Exception.h
    include/spark/Helpers.h
)

set(COREv2
    include/spark/v2/Server.h
    include/spark/v2/Handler.h
    include/spark/v2/PeerConnection.h
    include/spark/v2/PeerHandler.h
    include/spark/v2/RemotePeer.h
	include/spark/v2/Link.h
	include/spark/v2/MessageHeader.h
	include/spark/v2/Channel.h
	include/spark/v2/Tracker.h
	include/spark/v2/SharedDefs.h
	include/spark/v2/Message.h
	include/spark/v2/HandlerRegistry.h
    src/v2/Server.cpp
    src/v2/PeerHandler.cpp
	src/v2/RemotePeer.cpp
	src/v2/PeerConnection.cpp
	src/v2/Channel.cpp
	src/v2/Tracker.cpp
	src/v2/HandlerRegistry.cpp
)

set(IO_SRC
    include/spark/buffers/BufferBase.h
    include/spark/buffers/BufferRead.h
    include/spark/buffers/BufferWrite.h
    include/spark/buffers/Buffer.h
    include/spark/buffers/DynamicBuffer.h
    include/spark/buffers/detail/IntrusiveStorage.h
    include/spark/buffers/BufferSequence.h
    include/spark/buffers/NullBuffer.h
    include/spark/buffers/BinaryStream.h
    include/spark/buffers/BinaryStreamReader.h
    include/spark/buffers/BinaryStreamWriter.h
    include/spark/buffers/StreamBase.h
    include/spark/buffers/BufferAdaptor.h
    include/spark/buffers/BufferReadAdaptor.h
    include/spark/buffers/BufferWriteAdaptor.h
)

set(IO_SRCv2
    include/spark/v2/buffers/BufferAdaptor.h
    include/spark/v2/buffers/BinaryStream.h
)


add_library(
    ${LIBRARY_NAME}
    ${CORE}
    ${COREv2}
    ${IO_SRC}
    ${IO_SRCv2}
)

source_group("Core" FILES ${CORE})
source_group("IO" FILES ${IO_SRC})
source_group("v2\\Core" FILES ${COREv2})
source_group("v2\\IO" FILES ${IO_SRCv2})

add_dependencies(${LIBRARY_NAME} FB_SCHEMA_COMPILE)
target_link_libraries(${LIBRARY_NAME} shared ${Boost_LIBRARIES})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(spark PROPERTIES FOLDER "Libraries")