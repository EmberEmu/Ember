# Copyright (c) 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME bzip2)

add_library(${LIBRARY_NAME} STATIC
	blocksort.c
	bzlib.c
	compress.c
	crctable.c
	decompress.c
	huffman.c
	randtable.c
	bzlib.h
)

set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Dependencies")