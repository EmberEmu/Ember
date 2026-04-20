/*
 * Copyright (c) 2026 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

 // number of buffer nodes to preallocate, per thread
#ifndef PREALLOCATED_NODES_PER_THREAD
#define PREALLOCATED_NODES_PER_THREAD 0
#endif 

 // number of clients to preallocate, per thread
#ifndef PREALLOCATED_CLIENTS_PER_THREAD
#define PREALLOCATED_CLIENTS_PER_THREAD 0
#endif

 // size of the inbound client buffer
#ifndef CLIENT_BUFFER_IN_SIZE
#define CLIENT_BUFFER_IN_SIZE 8192
#endif

 // size of the outbound client buffers
#ifndef CLIENT_BUFFER_OUT_SIZE
#define CLIENT_BUFFER_OUT_SIZE 8192
#endif

 // maximum size of the outbound client buffers
#ifndef CLIENT_BUFFER_OUT_MAX_SIZE
#define CLIENT_BUFFER_OUT_MAX_SIZE 65536
#endif

// the maximum chunk size Asio can request without resorting to the system allocator
#ifndef PREALLOCATED_ASIO_CHUNK_SIZE
#define PREALLOCATED_ASIO_CHUNK_SIZE 512
#endif

// the number of preallocated chunks Asio can request without using the system allocator
#ifndef PREALLOCATED_ASIO_CHUNKS 
#define PREALLOCATED_ASIO_CHUNKS 8
#endif
