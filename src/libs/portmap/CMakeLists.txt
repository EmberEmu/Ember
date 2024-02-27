# Copyright (c) 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME portmap)

add_library(${LIBRARY_NAME}
           src/natpmp/Client.cpp
		   src/natpmp/DatagramTransport.cpp
           include/portmap/Client.h
           include/portmap/natpmp/Client.h
		   include/portmap/natpmp/DatagramTransport.h
           include/portmap/natpmp/Protocol.h
           include/portmap/natpmp/Serialise.h
           include/portmap/natpmp/Deserialise.h
           )

target_link_libraries(${LIBRARY_NAME} shared spark ${Boost_LIBRARIES})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(portmap PROPERTIES FOLDER "Libraries")