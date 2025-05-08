# Copyright (c) 2025 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Use vswhere.exe to locate the latest Visual Studio installation that includes the VC Tools.
$vsPath = & "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" `
          -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath

# Build the full path to the VsDevCmd.bat from the installation path.
$vsDevCmd = Join-Path $vsPath "Common7\Tools\VsDevCmd.bat"

# Use the developer command prompt to set up the environment.
& "$vsDevCmd"

#############################################################
# Install dependencies through vcpkg
# Just grab and bootstrap the vcpkg and the toolchain file will handle the rest
#############################################################
Write-Host "Cloning vcpkg and boot-strapping"
git clone https://github.com/microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg integrate install

###############################
# Configure and Build Ember
###############################
Write-Host "=== Configuring project with CMake ==="

# Check if we are running on an ARM architecture.
if ($env:PROCESSOR_ARCHITECTURE -eq "ARM64") {
    Write-Host "ARM64 architecture detected on this build runner."
    $targetTriplet= "arm64-windows-static"
} else {
    Write-Host "Non-ARM architecture detected."
    $targetTriplet = "x64-windows-static"
}

$buildDir            = "build"
$installDir          = ".\build\bin"
$generator           = "Visual Studio 17 2022"
$toolchainFile       = "vcpkg\scripts\buildsystems\vcpkg.cmake"
$buildOptionalTools  = "-1"
$disableEmberThreads = "0"
$runtimeOption       = "MultiThreaded$<$<CONFIG:Debug>:Debug>"
$buildType           = "Debug"

cmake -S . -B $buildDir -G "$generator" `
      -DCMAKE_TOOLCHAIN_FILE="$toolchainFile" `
      -DVCPKG_TARGET_TRIPLET="$targetTriplet" `
      -DCMAKE_MSVC_RUNTIME_LIBRARY="$runtimeOption" `
      -DBUILD_OPT_TOOLS="$buildOptionalTools" `
      -DDISABLE_EMBER_THREADS="$disableEmberThreads" `
      -DCMAKE_INSTALL_PREFIX="$installDir"

Write-Host "Building and installing the project..."
cmake --build $buildDir --target install --config "$buildType"

###############################################
# Run the unit_tests for regression control
###############################################
Write-Host "=== Switching to installed directory and running tests ==="
Set-Location $installDir
if (Test-Path ".\unit_tests.exe") {
    .\unit_tests.exe
} else {
    Write-Host "Warning: Installed test executable not found."
}

Write-Host "=== Build, install, and test complete ==="
