# Copyright (c) 2015 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_library(logging
            src/Logging.cpp
            src/FileSink.cpp
            src/SyslogSink.cpp
            src/Logger.cpp
            src/Utility.cpp
            src/Worker.cpp
            src/ConsoleSink.cpp
            include/logger/concurrentqueue.h
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
)

target_link_libraries(logging shared ${Boost_LIBRARIES})
target_include_directories(logging PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
