# Copyright (c) 2015 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME shared)

set(DB_OBJECTS
    shared/database/objects/User.h
)

set(SHARED_DAOS_SRC
    shared/database/Exception.h
    shared/database/daos/IPBanDAO.h
    shared/database/daos/RealmDAO.h
    shared/database/daos/UserDAO.h
    shared/database/daos/shared_base/IPBanBase.h
    shared/database/daos/shared_base/RealmBase.h
    shared/database/daos/shared_base/UserBase.h
)

set(MYSQL_DAOS_SRC
    shared/database/daos/mysql/IPBanDAO.h
    shared/database/daos/mysql/RealmDAO.h
    shared/database/daos/mysql/UserDAO.h
)

set(POSTGRESQL_DAOS_SRC
    shared/database/daos/postgresql/IPBanDAO.h
    shared/database/daos/postgresql/RealmDAO.h
    shared/database/daos/postgresql/UserDAO.h
)

set(MEMORY_SRC
    shared/memory/ASIOAllocator.h
)

set(THREADING_SRC
    shared/threading/Semaphore.h
    shared/threading/Spinlock.h
    shared/threading/ThreadPool.cpp
    shared/threading/ThreadPool.h
)

set(UTIL_SRC
    shared/util/FormatPacket.h
    shared/util/LogConfig.cpp
    shared/util/LogConfig.h
)

set(LIBRARY_SRC
    ${DB_OBJECTS}
    ${SHARED_DAOS_SRC}
    ${MYSQL_DAOS_SRC}
    ${POSTGRESQL_DAOS_SRC}
    ${MEMORY_SRC}
    ${THREADING_SRC}
    ${UTIL_SRC}
    shared/CompilerWarn.h
    shared/IPBanCache.h
    shared/PacketStream.h
    shared/Realm.h
    shared/rlutil.h
    shared/Banner.cpp
    shared/Banner.h
    shared/Version.h
)

source_group("Database Objects" FILES ${DB_OBJECTS})
source_group("DAO base" FILES ${SHARED_DAOS_SRC})
source_group("MySQL DAO" FILES ${MYSQL_DAOS_SRC})
source_group("PostgreSQL DAO" FILES ${POSTGRESQL_DAOS_SRC})
source_group("Memory" FILES ${MEMORY_SRC})
source_group("Threading" FILES ${THREADING_SRC})
source_group("Utilities" FILES ${UTIL_SRC})

add_library(${LIBRARY_NAME} ${LIBRARY_SRC})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/)
target_link_libraries(${LIBRARY_NAME} logging)