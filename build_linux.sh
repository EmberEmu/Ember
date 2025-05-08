# Copyright (c) 2025 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

#!/bin/bash
set -e

#####################################################################################################
# Install build tools
# Already pre-installed: (software-properties-common, wget, gcc-14, g++-14, libstdc++-14-dev, git)
# GCC is already the default compiler and up-to-date - libtirpc-dev is for libmysqlconncpp via vcpkg
#####################################################################################################
echo "installing apt-get dependencies..."
sudo apt-get install -y build-essential cmake
sudo update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-14 100
sudo update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-14 100
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 100
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 100

###################################################################################
# Install homebrew and get dependencies through here until available in apt-get
###################################################################################
sudo apt-get install -y procps curl file libpcre3-dev

/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

echo 'eval "$(/home/linuxbrew/.linuxbrew/bin/brew shellenv)"' >>~/.profile
eval "$(/home/linuxbrew/.linuxbrew/bin/brew shellenv)"

brew update
brew upgrade
brew install boost
brew install botan
brew install mysql-client
brew install flatbuffers

#######################################
# Install MySQL Connector/C++
#######################################
CACHE_TARBALL="dependencies/mysql-connector.tar.gz"

if [ -f "$CACHE_TARBALL" ]; then
  echo "Cached MySQL Connector tarball found."
else
  arch=$(uname -m) && case "$arch" in \
    x86_64) url="https://dev.mysql.com/get/Downloads/Connector-C++/mysql-connector-c++-9.3.0-linux-glibc2.28-x86-64bit.tar.gz" ;; \
    aarch64) url="https://dev.mysql.com/get/Downloads/Connector-C++/mysql-connector-c++-9.3.0-linux-glibc2.28-aarch64.tar.gz" ;; \
    *) echo "Unsupported architecture: $arch" && exit 1 ;; esac
  echo "Downloading MySQL Connector/C++ from ${url}"
  sudo mkdir -p dependencies
  sudo wget "$url" -O "$CACHE_TARBALL"
fi

echo "Extracting MySQL Connector tarball..."
sudo mkdir -p /usr/lib/cmake/mysql-concpp
sudo tar -zxf "$CACHE_TARBALL" -C /usr/lib/cmake/mysql-concpp --strip-components=1

echo "Installing headers and libraries..."
sudo mkdir -p /usr/include/mysql-cppconn
sudo cp -r /usr/lib/cmake/mysql-concpp/include/* /usr/include/mysql-cppconn/
sudo cp -r /usr/lib/cmake/mysql-concpp/lib64/* /usr/local/lib/
sudo ldconfig
echo "MySQL Connector/C++ installed."

###############################
# Configure and Build Ember
###############################
echo "=== Configuring project with CMake ==="

sudo rm -rf build

BUILD_OPTIONAL_TOOLS=-1
DISABLE_THREADS=0
BUILD_DIR="build"
INSTALL_DIR="./build/bin"
BUILD_TYPE="Debug"

cmake -S . -B ${BUILD_DIR} \
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
  ./unit_tests
else
  echo "Error: Installed test executable not found in ${INSTALL_DIR}. Aborting."
  exit 1
fi

echo "=== Build, install, and test complete ==="
