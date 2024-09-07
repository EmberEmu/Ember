# Copyright (c) 2015 - 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME logger)

add_library(${LIBRARY_NAME}
            src/FileSink.cpp
            src/SyslogSink.cpp
            src/Logger.cpp
            src/Utility.cpp
            src/Worker.cpp
            src/ConsoleSink.cpp
            src/GlobalLogger.cpp
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
            include/logger/Severity.h
            include/logger/GlobalLogger.h
)

target_link_libraries(${LIBRARY_NAME} shared ${Boost_LIBRARIES} Threads::Threads)
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(logger PROPERTIES FOLDER "Libraries")