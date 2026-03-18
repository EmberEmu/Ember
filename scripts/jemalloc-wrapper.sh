#!/bin/bash

# This script runs the specified service after setting default jemalloc
# options that prioritise CPU over memory usage
#
# Has no effect if the service was not compiled with jemalloc

if [ -z "$1" ]; then
  echo "Usage: $0 <program_name>"
  exit 1
fi

echo "Running $1 with jemalloc tuned for CPU (higher memory usage)..."
export MALLOC_CONF=background_thread:true,metadata_thp:auto,percpu_arena:percpu,dirty_decay_ms:30000,muzzy_decay_ms:30000
"$1"