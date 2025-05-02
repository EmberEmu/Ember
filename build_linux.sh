#!/bin/bash
set -e

################################################################################
# This script replicates the "builder" stage from your Dockerfile for Ubuntu,
# preserving the original functionality exactly as it was (except botan, MySQL,
# and related packages have been removed).
#
# The script does the following:
#
# 1. Updates the system and installs required tools:
#      - software-properties-common, wget, build-essential, gcc-14/g++-14,
#        libstdc++-14-dev, cmake, and git.
#
# 2. Sets gcc-14 and g++-14 as the default compilers using update-alternatives.
#
# 3. Assumes the entire source tree is already checked out in the working directory.
#
# 4. Configures the project using CMake in a local "build" directory,
#    using the cached shared vcpkg toolchain file (which must be at
#    vcpkg/scripts/buildsystems/vcpkg.cmake in the repository root).
#
# 5. Builds the project and installs it into INSTALL_DIR.
#
# 6. Changes directory to INSTALL_DIR and immediately runs the installed test
#    executable (without copying or creating any extra directories).
################################################################################

echo "Updating system and installing dependencies..."
sudo apt-get update -y && sudo apt-get upgrade -y
sudo apt-get install -y software-properties-common wget build-essential gcc-14 g++-14 libstdc++-14-dev cmake git

echo "Setting up gcc-14 as default compiler..."
sudo update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-14 100
sudo update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-14 100
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 100
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 100

# Set the working directory (assumes the repo is checked out here)
WORKING_DIR=$(pwd)
echo "Working directory: ${WORKING_DIR}"

# Read build configuration from environment variables (or use defaults)
BUILD_OPTIONAL_TOOLS=${BUILD_OPTIONAL_TOOLS:-1}
DISABLE_THREADS=${DISABLE_THREADS:-0}
BUILD_TYPE=${BUILD_TYPE:-Rel}
INSTALL_DIR=${INSTALL_DIR:-./build/bin}

# Ensure the shared vcpkg repository is available.
# (Your workflow should have already ensured that the vcpkg folder is cached.)
VCPKG_TOOLCHAIN_FILE="./vcpkg/scripts/buildsystems/vcpkg.cmake"
if [ ! -f "${VCPKG_TOOLCHAIN_FILE}" ]; then
  echo "Error: vcpkg toolchain file not found at ${VCPKG_TOOLCHAIN_FILE}"
  exit 1
fi

################################################################################
echo "=== Configuring project with CMake ==="
mkdir -p build
cd build

cmake -S .. -B . \
  -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
  -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
  -DBUILD_OPT_TOOLS=${BUILD_OPTIONAL_TOOLS} \
  -DCMAKE_TOOLCHAIN_FILE="${VCPKG_TOOLCHAIN_FILE}" \
  -DDISABLE_EMBER_THREADS=${DISABLE_THREADS}

################################################################################
echo "=== Building and Installing project ==="
make -j$(nproc) install

################################################################################
echo "=== Switching to installed directory and running tests ==="
cd ${INSTALL_DIR}

# Run the installed test executable from the install directory.
# Replace "./ember_test" with your actual installed test command as needed.
if [ -x "./ember_test" ]; then
  echo "Running installed test executable..."
  ./ember_test
else
  echo "Error: Installed test executable not found in ${INSTALL_DIR}. Aborting."
  exit 1
fi

echo "Build, install, and test complete."
