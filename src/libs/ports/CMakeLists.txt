# Copyright (c) 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME ports)

add_library(${LIBRARY_NAME}
           src/pcp/Client.cpp
		   src/pcp/DatagramTransport.cpp
		   src/pcp/Daemon.cpp
		   src/upnp/SSDP.cpp
		   src/upnp/MulticastSocket.cpp
		   src/upnp/IGDevice.cpp
		   src/upnp/HTTPTransport.cpp
		   src/upnp/XMLParser.cpp
           include/ports/pcp/Client.h
		   include/ports/pcp/Results.h
		   include/ports/pcp/Daemon.h
		   include/ports/pcp/DatagramTransport.h
           include/ports/pcp/Protocol.h
           include/ports/pcp/Serialise.h
           include/ports/pcp/Deserialise.h
		   include/ports/upnp/SSDP.h
		   include/ports/upnp/MulticastSocket.h
		   include/ports/upnp/HTTPHeaderParser.h
		   include/ports/upnp/IGDevice.h
		   include/ports/upnp/HTTPTransport.h
		   include/ports/upnp/XMLParser.h
		   include/ports/upnp/XMLVisitor.h
		   include/ports/upnp/Utility.h
           )

include_directories(${CMAKE_SOURCE_DIR}/deps/rapidxml)
target_link_libraries(${LIBRARY_NAME} shared spark ${Boost_LIBRARIES})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(ports PROPERTIES FOLDER "Libraries")