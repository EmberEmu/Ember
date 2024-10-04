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
    include/spark/v2/Connection.h
    include/spark/v2/RemotePeer.h
    include/spark/v2/Link.h
    include/spark/v2/MessageHeader.h
    include/spark/v2/Channel.h
    include/spark/v2/HandlerRegistry.h
    include/spark/v2/Peers.h
    include/spark/v2/Utility.h
    include/spark/v2/Tracking.h
    include/spark/v2/Common.h
    include/spark/v2/Result.h
    src/v2/Peers.cpp
    src/v2/Server.cpp
    src/v2/RemotePeer.cpp
    src/v2/Connection.cpp
    src/v2/Channel.cpp
    src/v2/HandlerRegistry.cpp
    src/v2/Tracking.cpp
)

set(IO_SRC
    include/spark/buffers/Exception.h
    include/spark/buffers/DynamicBuffer.h
    include/spark/buffers/DynamicTLSBuffer.h
    include/spark/buffers/IntrusiveStorage.h
    include/spark/buffers/SharedDefs.h
    include/spark/buffers/BufferAdaptor.h
    include/spark/buffers/BufferSequence.h
    include/spark/buffers/BinaryStream.h
    include/spark/buffers/StaticBuffer.h
)

set(IO_PMR_SRC
    include/spark/buffers/pmr/BufferBase.h
    include/spark/buffers/pmr/BufferRead.h
    include/spark/buffers/pmr/BufferWrite.h
    include/spark/buffers/pmr/Buffer.h
    include/spark/buffers/pmr/NullBuffer.h
    include/spark/buffers/pmr/BinaryStream.h
    include/spark/buffers/pmr/BinaryStreamReader.h
    include/spark/buffers/pmr/BinaryStreamWriter.h
    include/spark/buffers/pmr/StreamBase.h
    include/spark/buffers/pmr/BufferAdaptor.h
    include/spark/buffers/pmr/BufferReadAdaptor.h
    include/spark/buffers/pmr/BufferWriteAdaptor.h
)

set(IO_ALLOC_SRC
    include/spark/buffers/allocators/DefaultAllocator.h
    include/spark/buffers/allocators/TLSBlockAllocator.h
)

add_library(
    ${LIBRARY_NAME}
    ${CORE}
    ${COREv2}
    ${IO_SRC}
    ${IO_PMR_SRC}
	${IO_ALLOC_SRC}
)

source_group("Core" FILES ${CORE})
source_group("IO" FILES ${IO_SRC})
source_group("IO\\Polymorphic" FILES ${IO_PMR_SRC})
source_group("IO\\Allocators" FILES ${IO_ALLOC_SRC})
source_group("v2\\Core" FILES ${COREv2})

add_dependencies(${LIBRARY_NAME} FB_SCHEMA_COMPILE)
target_link_libraries(${LIBRARY_NAME} shared ${Boost_LIBRARIES})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(spark PROPERTIES FOLDER "Libraries")