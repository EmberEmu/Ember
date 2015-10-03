# Copyright (c) 2015 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_library(conpool
            src/MySQLDriver.cpp
            include/conpool/ConnectionPool.h
            include/conpool/PoolManager.h
            include/conpool/Connection.h
            include/conpool/Policies.h
            include/conpool/Exception.h
            include/conpool/LogSeverity.h
            include/conpool/drivers/AutoSelect.h
            include/conpool/drivers/MySQLDriver.h
           )

target_include_directories(conpool PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)