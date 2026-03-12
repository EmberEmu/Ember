# Copyright (c) 2023 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME stun)

add_library(${LIBRARY_NAME} STATIC
    include/stun/Client.h
    include/stun/TransportBase.h
    include/stun/DatagramTransport.h
    include/stun/StreamTransport.h
    include/stun/Protocol.h
    include/stun/Logging.h
    include/stun/Attributes.h
    include/stun/Transaction.h
    include/stun/Parser.h
    include/stun/Parser.inl
    include/stun/Utility.h
    include/stun/Exception.h
    include/stun/detail/Shared.h
    include/stun/MessageBuilder.h
    include/stun/LogSeverity.h
    src/Client.cpp
    src/DatagramTransport.cpp
    src/StreamTransport.cpp
    src/Protocol.cpp
    src/Parser.cpp
    src/MessageBuilder.cpp
    src/detail/Shared.cpp
)

target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${LIBRARY_NAME} PRIVATE thread PUBLIC shared spark botan::botan ${Boost_LIBRARIES})
set_target_properties(${LIBRARY_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Common Libraries")