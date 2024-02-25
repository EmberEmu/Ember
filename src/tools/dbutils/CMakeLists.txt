# Copyright (c) 2019 - 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(EXECUTABLE_NAME dbutils)

# added so the files so up in an IDE's browser
set(EXECUTABLE_HDR
    DatabaseDetails.h
    QueryExecutor.h
    mysql/MySQLQueryExecutor.h
    )

set(EXECUTABLE_SRC
    main.cpp
    mysql/MySQLQueryExecutor.cpp
    )

add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_HDR} ${EXECUTABLE_SRC} ${version_file})
target_link_libraries(${EXECUTABLE_NAME} logging shared conpool ${MYSQLCCPP_LIBRARY} ${Boost_LIBRARIES} Threads::Threads)
INSTALL(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/tools)
set_target_properties(dbutils PROPERTIES FOLDER "Tools")