# Copyright (c) 2024 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(LIBRARY_NAME lzma)

add_library(${LIBRARY_NAME} STATIC
	LzFind.c
	Lzma2Dec.c
	Lzma2Enc.c
	LzmaDec.c
	LzmaEnc.c
	LzmaLib.c
	CpuArch.c
	LzFindMt.c
	MtDec.c
	Alloc.c
	Threads.c
	LzmaLib.h
	MtCoder.c
	LzFindOpt.c
)

set_target_properties(${LIBRARY_NAME} PROPERTIES FOLDER "Dependencies")