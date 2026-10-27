# Copyright (c) 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include(CheckCSourceCompiles)

check_c_source_compiles("
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) \
	|| defined(__x86_64__) || defined(__amd64__) || defined(_M_AMD64)
int main(void) { return 0; }
#else
#error Not an x86/x64 target
#endif
" XXHASH_TARGET_IS_X86_X64)

set(LIBRARY_NAME xxhash)

set(LIBRARY_SOURCES
    xxhash.c
    xxhash.h
)
if(XXHASH_TARGET_IS_X86_X64)
    list(APPEND LIBRARY_SOURCES
        xxh_x86dispatch.c
        xxh_x86dispatch.h
    )
endif()

add_library(${LIBRARY_NAME} STATIC ${LIBRARY_SOURCES})

set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Dependencies")