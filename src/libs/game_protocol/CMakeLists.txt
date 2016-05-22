# Copyright (c) 2016 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME game_protocol)

add_library(${LIBRARY_NAME}
            src/Dummy.cpp
            include/game_protocol/Opcodes.h
            include/game_protocol/Packet.h
            include/game_protocol/Packets.h
            include/game_protocol/ResultCodes.h
            include/game_protocol/PacketHeaders.h
)

target_link_libraries(${LIBRARY_NAME} shared spark ${Boost_LIBRARIES})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
