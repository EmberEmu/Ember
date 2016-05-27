# Copyright (c) 2016 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME taskmaster)

set(NDSCHEDULER_SRC
    include/ndscheduler/Scheduler.h
    src/ndscheduler/Scheduler.cpp
)

set(WSSCHEDULER_SRC
    include/wsscheduler/Common.h
    include/wsscheduler/Task.h
    include/wsscheduler/Scheduler.h
    include/wsscheduler/Worker.h
    include/wsscheduler/Dequeue.h
    src/wsscheduler/Scheduler.cpp
    src/wsscheduler/Worker.cpp
    src/wsscheduler/Dequeue.cpp
)

set(LIBRARY_SRC ${NDSCHEDULER_SRC} ${WSSCHEDULER_SRC})

source_group("ND Scheduler" FILES ${NDSCHEDULER_SRC})
source_group("WS Scheduler" FILES ${WSSCHEDULER_SRC})

add_library(${LIBRARY_NAME} ${LIBRARY_SRC})

target_link_libraries(${LIBRARY_NAME} logger shared)
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)