# Copyright (c) 2015 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_library(logging
            FileSink.cpp
            SyslogSink.cpp
            Logger.cpp
            Utility.cpp
            Worker.cpp
            ConsoleSink.cpp
            Sink.cpp
            concurrentqueue.h)

target_include_directories(logging PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)