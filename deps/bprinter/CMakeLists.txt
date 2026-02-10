# Copyright (c) 2026 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME bprinter)

add_library(${LIBRARY_NAME} STATIC
    table_printer.cpp
    table_printer.h
	impl/table_printer.tpp.h
)

set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Dependencies")