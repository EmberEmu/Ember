# Copyright (c) 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.30)
project(blazeas)

##############################
#         ANGELSCRIPT        #
##############################
find_package(Angelscript 2.38.0 REQUIRED)

add_subdirectory(src)