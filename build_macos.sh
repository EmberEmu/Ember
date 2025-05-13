# Copyright (c) 2025 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

#!/bin/bash
set -e

BUILD_DIR="build"
INSTALL_DIR="./build/bin"
BUILD_TYPE="Debug"

BUILD_OPTIONAL_TOOLS=-1
DISABLE_THREADS=0

# Force xcode 16.3 or it'll default to 16.0
sudo xcode-select -s "/Applications/Xcode_16.3.app/Contents/Developer"

###############################################
# Install build tools
# Already pre-installed: (git, cmake, ninja)
###############################################
brew update
brew upgrade
brew install llvm

#############################################################
# Install dependencies through homebrew 
# (Github macOS runner already come with openssl and zlib)
#############################################################
brew install boost
brew install botan
brew install mysql-client
brew install pcre
brew install flatbuffers

#######################################
# Install MySQL Connector/C++
#######################################
CACHE_TARBALL="dependencies/mysql-connector.tar.gz"

if [ -f "$CACHE_TARBALL" ]; then
  echo "Cached MySQL Connector tarball found."
else
  echo "Downloading MySQL Connector/C++ Library from Oracle..."
  sudo mkdir -p dependencies
  sudo wget https://dev.mysql.com/get/Downloads/Connector-C++/mysql-connector-c++-9.3.0-macos15-arm64.tar.gz -O "$CACHE_TARBALL"
fi

echo "Extracting MySQL Connector tarball..."
sudo mkdir -p /usr/local/lib/cmake/mysql-concpp
sudo tar -zxf "$CACHE_TARBALL" -C /usr/local/lib/cmake/mysql-concpp --strip-components=1

echo "Installing headers and libraries..."
sudo mkdir -p /usr/local/include/mysql-concpp
sudo cp -r /usr/local/lib/cmake/mysql-concpp/include/. /usr/local/include/mysql-concpp/
sudo cp -r /usr/local/lib/cmake/mysql-concpp/lib64/. /usr/local/lib/
echo "MySQL Connector/C++ installed."

echo "Patching MySQL Connector dependencies to reference absolute OpenSSL paths..."
for libfile in /usr/local/lib/libmysqlcppconn*.dylib; do
  if [ -f "$libfile" ]; then
    sudo install_name_tool -change libssl.3.dylib /opt/homebrew/opt/openssl@3/lib/libssl.3.dylib "$libfile"
    sudo install_name_tool -change libcrypto.3.dylib /opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib "$libfile"
    echo "Patched $libfile"
  fi
done

###############################
# Configure and Build Ember
###############################
echo "=== Configuring project with CMake ==="

# Set the DYLD_LIBRARY_PATH for all subsequent build and runtime steps.
export DYLD_LIBRARY_PATH="$(brew --prefix openssl@3)/lib:$DYLD_LIBRARY_PATH"
echo "DYLD_LIBRARY_PATH set to: $DYLD_LIBRARY_PATH"

C_COMPILER="/opt/homebrew/opt/llvm/bin/clang"
CXX_COMPILER="/opt/homebrew/opt/llvm/bin/clang++"

CMAKE_C_FLAGS="-isystem /opt/homebrew/opt/llvm/include/c++/v1 -isysroot $(xcrun --show-sdk-path)"
CMAKE_CXX_FLAGS="-isystem /opt/homebrew/opt/llvm/include/c++/v1 -isysroot $(xcrun --show-sdk-path)"

cmake -S . -B ${BUILD_DIR} \
  -DCMAKE_OSX_SYSROOT=$(xcrun --show-sdk-path) \
  -DCMAKE_C_COMPILER=${C_COMPILER} \
  -DCMAKE_CXX_COMPILER=${CXX_COMPILER} \
  -DCMAKE_C_FLAGS="${CMAKE_C_FLAGS}" \
  -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS}" \
  -DBUILD_OPT_TOOLS=${BUILD_OPTIONAL_TOOLS} \
  -DDISABLE_EMBER_THREADS=${DISABLE_THREADS} \
  -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}

echo "Building and installing the project..."
cmake --build ${BUILD_DIR} --target install --config ${BUILD_TYPE}

###############################################
# Run the unit_tests for regression control
###############################################
echo "=== Switching to installed directory and running tests ==="
cd ${INSTALL_DIR}
if [ -x "./unit_tests" ]; then
  echo "Running installed test executable..."
  # openssl is linked dynamically so we need to point the unit_tests to the library
  sudo install_name_tool -change libssl.3.dylib "$(brew --prefix openssl@3)/lib/libssl.3.dylib" ./unit_tests
  sudo install_name_tool -change libcrypto.3.dylib "$(brew --prefix openssl@3)/lib/libcrypto.3.dylib" ./unit_tests
  sudo -E ./unit_tests
else
  echo "Error: Installed test executable not found in ${INSTALL_DIR}. Aborting."
  exit 1
fi

echo "=== Build, install, and test complete ==="
