# Copyright (c) 2015 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_library(logging
            Logging.cpp
            FileSink.cpp
            SyslogSink.cpp
            Logger.cpp
            Utility.cpp
            Worker.cpp
            ConsoleSink.cpp
            Sink.cpp
            concurrentqueue.h
            include/logger/LoggerImpl.h
            include/logger/FileSink.h
            include/logger/HelperMacros.h
            include/logger/SyslogSink.h
            include/logger/Worker.h
            include/logger/Logger.h
            include/logger/Sink.h
            include/logger/Utility.h
            include/logger/ConsoleSink.h
            include/logger/Exception.h
            include/logger/FileWrapper.h
            include/logger/Logging.h
            include/logger/Severity.h
            ../threading/Semaphore.h
)

target_include_directories(logging PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)