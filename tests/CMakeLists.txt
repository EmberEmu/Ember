# Copyright (c) 2014 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.


set(EXECUTABLE_SRC
    srp6.cpp
    )

add_executable(unit_tests ${EXECUTABLE_SRC})
target_link_libraries(unit_tests ${Boost_LIBRARIES})
target_link_libraries(unit_tests ${BOTAN_LIBRARY})
target_link_libraries(unit_tests srp6)
target_include_directories(unit_tests PRIVATE ../src)
target_link_libraries(unit_tests gtest gtest_main)
add_test(unit_tests unit_tests)
