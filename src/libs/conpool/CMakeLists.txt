# Copyright (c) 2015 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME conpool)

add_library(${LIBRARY_NAME} STATIC
    src/MySQL/Driver.cpp
    src/MySQL/Config.cpp
    src/DummyDriver.cpp
    include/conpool/PoolImpl.h
    include/conpool/PoolManager.h
    include/conpool/Connection.h
    include/conpool/Policies.h
    include/conpool/Exception.h
    include/conpool/LogSeverity.h
    include/conpool/ConnectionPool.h
    include/conpool/drivers/AutoSelect.h
    include/conpool/drivers/MySQL/Driver.h
    include/conpool/drivers/MySQL/Config.h
    include/conpool/drivers/DummyDriver.h
)

target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${LIBRARY_NAME} PUBLIC thread shared ${MYSQLCCPP_LIBRARY})
set_target_properties(${LIBRARY_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Common Libraries")