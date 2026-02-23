# Copyright (c) 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME commands)

add_library(${LIBRARY_NAME} STATIC
    src/Registry.cpp
    src/Command.cpp
	src/TypeMap.cpp
    src/ScopedCommand.cpp
    include/commands/Registry.h
    include/commands/Command.h
    include/commands/Argument.h
    include/commands/Utility.h
    include/commands/Result.h
    include/commands/Suggestions.h
    include/commands/Exception.h
    include/commands/ArgumentMap.h
    include/commands/ArgumentType.h
    include/commands/ArgumentValue.h
    include/commands/PrefixedRegistry.h
    include/commands/ScopedCommand.h
    include/commands/UserData.h
)

target_link_libraries(${LIBRARY_NAME} PUBLIC shared ${Boost_LIBRARIES})
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Common Libraries")