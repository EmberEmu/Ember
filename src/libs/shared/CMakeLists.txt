# Copyright (c) 2015 - 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME shared)

set(DB_OBJECTS
    shared/database/objects/User.h
    shared/database/objects/Character.h
    shared/database/objects/PatchMeta.h
)

set(SHARED_DAOS_SRC
    shared/database/Exception.h
    shared/database/daos/IPBanDAO.h
    shared/database/daos/RealmDAO.h
    shared/database/daos/UserDAO.h
    shared/database/daos/CharacterDAO.h
    shared/database/daos/PatchDAO.h
    shared/database/daos/base/IPBan.h
    shared/database/daos/base/Realm.h
    shared/database/daos/base/User.h
    shared/database/daos/base/Character.h
    shared/database/daos/base/Patch.h
)

set(MYSQL_DAOS_SRC
    shared/database/daos/mysql/IPBanDAO.h
    shared/database/daos/mysql/RealmDAO.h
    shared/database/daos/mysql/UserDAO.h
    shared/database/daos/mysql/CharacterDAO.h
    shared/database/daos/mysql/PatchDAO.h
)

set(POSTGRESQL_DAOS_SRC
    shared/database/daos/postgresql/IPBanDAO.h
    shared/database/daos/postgresql/RealmDAO.h
    shared/database/daos/postgresql/UserDAO.h
    shared/database/daos/postgresql/CharacterDAO.h
    shared/database/daos/postgresql/PatchDAO.h
)

set(MEMORY_SRC
    shared/memory/AsioAllocator.h
)

set(THREADING_SRC
    shared/threading/Semaphore.h
    shared/threading/Spinlock.h
    shared/threading/ThreadPool.cpp
    shared/threading/ThreadPool.h
    shared/threading/Utility.h
    shared/threading/Utility.cpp
    shared/threading/ServicePool.h
    shared/threading/ServicePool.cpp
)

set(UTIL_SRC
    shared/utility/xoroshiro128plus.h
    shared/utility/xoroshiro128plus.cpp
    shared/utility/enum_bitmask.h
    shared/utility/PCREHelper.h
    shared/utility/PCREHelper.cpp
    shared/utility/UTF8String.h
    shared/utility/UTF8.h
    shared/utility/UTF8.cpp
    shared/utility/Utility.h
    shared/utility/Utility.cpp
    shared/utility/FormatPacket.h
    shared/utility/LogConfig.cpp
    shared/utility/LogConfig.h
    shared/utility/ConsoleColour.h
    shared/utility/ConsoleColour.cpp
    shared/utility/base32.h
    shared/utility/base32.cpp
    shared/utility/FileMD5.h
    shared/utility/FileMD5.cpp
    shared/utility/FNVHash.h
    shared/utility/EnumHelper.h
    shared/utility/MulticharConstant.h
    shared/utility/Clock.h
    shared/utility/StringHash.h
    shared/utility/STUN.h
    shared/utility/polyfill/print
    shared/utility/polyfill/start_lifetime_as
    shared/utility/polyfill/inplace_vector
    shared/utility/polyfill/ranged_iota
    shared/utility/polyfill/atomic_shared_ptr
    shared/utility/Timing.h
    shared/utility/Timing.cpp
    shared/utility/Exception.h
    shared/utility/CommandHelpers.h
    shared/utility/CommandHelpers.cpp
)

set(METRICS_SRC
    shared/metrics/Metrics.h
    shared/metrics/MetricsImpl.h
    shared/metrics/MetricsImpl.cpp
    shared/metrics/Monitor.h
    shared/metrics/Monitor.cpp
    shared/metrics/MetricsPoll.h
    shared/metrics/MetricsPoll.cpp
)

set(COMMANDS_SRC
    shared/commands/Registry.h
    shared/commands/Registry.cpp
    shared/commands/Command.h
    shared/commands/Command.cpp
    shared/commands/Argument.h
    shared/commands/Arguments.h
    shared/commands/Utility.h
    shared/commands/Result.h
    shared/commands/Suggestions.h
    shared/commands/Exception.h
    shared/commands/ArgumentType.h
    shared/commands/ArgumentValue.h
    shared/commands/PrefixedRegistry.h
)

set(GAME_SRC
    shared/game/GameVersion.h
    shared/game/Utility.h
)

set(LIBRARY_SRC
    ${GAME_SRC}
    ${DB_OBJECTS}
    ${SHARED_DAOS_SRC}
    ${MYSQL_DAOS_SRC}
    ${POSTGRESQL_DAOS_SRC}
    ${MEMORY_SRC}
    ${THREADING_SRC}
    ${UTIL_SRC}
    ${METRICS_SRC}
	${COMMANDS_SRC}
    shared/ClientRef.h
    shared/CompilerWarn.h
    shared/IPBanCache.h
    shared/Realm.h
    shared/smartenum.hpp
    shared/smartenum.cpp
)

source_group("Database Objects" FILES ${DB_OBJECTS})
source_group("DAO base" FILES ${SHARED_DAOS_SRC})
source_group("MySQL DAO" FILES ${MYSQL_DAOS_SRC})
source_group("PostgreSQL DAO" FILES ${POSTGRESQL_DAOS_SRC})
source_group("Memory" FILES ${MEMORY_SRC})
source_group("Threading" FILES ${THREADING_SRC})
source_group("Utilities" FILES ${UTIL_SRC})
source_group("Metrics" FILES ${METRICS_SRC})
source_group("Command Parser" FILES ${COMMANDS_SRC})

include_directories(${CMAKE_SOURCE_DIR}/deps/utf8cpp ${PCRE_INCLUDE_DIR} ${PROJECT_BINARY_DIR}/src)
add_library(${LIBRARY_NAME} STATIC ${LIBRARY_SRC})
target_include_directories(${LIBRARY_NAME} PUBLIC ${PCRE_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/)
target_link_libraries(${LIBRARY_NAME} PUBLIC logger conpool botan::botan-static ${PCRE_LIBRARY})
set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Common Libraries")