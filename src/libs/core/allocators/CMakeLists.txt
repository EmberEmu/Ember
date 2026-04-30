# Copyright (c) 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME allocators)

add_library(${LIBRARY_NAME} STATIC
    include/allocators/AsioAllocator.h
    include/allocators/PoolAllocator.h
    include/allocators/InplaceAllocator.h
    include/allocators/DefaultAllocator.h
    include/allocators/TLSBlockAllocator.h
    include/allocators/BlockAllocator.h
    include/allocators/Tracking.h
    include/allocators/AllocTrack.h
    Dummy.cpp
)

target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${LIBRARY_NAME} PUBLIC shared ${Boost_LIBRARIES})
set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Common Libraries")