#!/bin/bash
set -e

# build_macos.sh
# This script builds Ember on macOS.
# It installs required build dependencies via Homebrew (if needed),
# optionally installs a compiler, then configures and builds the project with CMake.
# It uses the shared, cached vcpkg repository (assumed to be at $HOME/vcpkg) via its
# toolchain file.

# --- Pre-requirements (assumed to have been manually installed) ---
# xcode-select --install
# /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
# brew update && brew upgrade

# --- Choose Compiler Strategy ---
# Option A: LLVM with libc++
echo "Using LLVM as compiler (with libc++)..."
brew install llvm
C_COMPILER="/opt/homebrew/opt/llvm/bin/clang"
CXX_COMPILER="/opt/homebrew/opt/llvm/bin/clang++"

# Option B: GCC with libstdc++ (uncomment below and comment out Option A if desired)
# echo "Using GCC as compiler (with libstdc++)..."
# brew install gcc
# C_COMPILER="/opt/homebrew/bin/gcc-14"
# CXX_COMPILER="/opt/homebrew/bin/g++-14"

# --- (Optional) Ensure CMake is in PATH ---
# If you need to add CMake from the CMake.app, update your PATH accordingly.
# export PATH="/Applications/CMake.app/Contents/bin:$PATH"
# source ~/.zshrc

# --- Configure Variables ---
buildDir="build"
installDir="./build/bin"  # Adjust if needed
toolchainFile="./vcpkg/scripts/buildsystems/vcpkg.cmake"  # Assumes the cached vcpkg repo is at $HOME/vcpkg

# --- Configure Project with CMake ---
echo "Configuring project with CMake..."
cmake -S . -B "$buildDir" \
  -DCMAKE_TOOLCHAIN_FILE="$toolchainFile" \
  -DCMAKE_C_COMPILER="$C_COMPILER" \
  -DCMAKE_CXX_COMPILER="$CXX_COMPILER" \
  -DCMAKE_INSTALL_PREFIX="$installDir"

# --- Build & Install ---
echo "Building and installing the project..."
cmake --build "$buildDir" --target install --config Debug

echo "Switching to installed directory and running tests..."
cd "$installDir"

if [ -x "./ember_test" ]; then
    echo "Running installed test executable..."
    ./ember_test
else
    echo "Error: Installed test executable not found in $installDir. Aborting."
    exit 1
fi
