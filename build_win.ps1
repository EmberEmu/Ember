# build_windows.ps1
# This script builds Ember on Windows using Visual Studio 17 2022.
# The runner already has Visual Studio Build Tools, Git, CMake, and Ninja pre-installed.
# It assumes the shared, cached vcpkg repository is at C:\vcpkg.
#
# Configure the project with CMake using the Visual Studio generator,
# then build and install the project twice (as required).

Write-Host "Using pre-installed Visual Studio Build Tools and dependencies."

# Optionally, if needed, you can initialize the VS environment:
# & "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"

$buildDir = "build"
$installDir = ".\build\bin"
$toolchainFile = ".\vcpkg\scripts\buildsystems\vcpkg.cmake" # From shared vcpkg repo
$generator = "Visual Studio 17 2022"
$platform = "x64"
$cmakeOptions = '-DVCPKG_TARGET_TRIPLET=x64-windows-static -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>"'

Write-Host "Configuring project with CMake..."
cmake -S . -B $buildDir -G "$generator" -A $platform -DCMAKE_TOOLCHAIN_FILE="$toolchainFile" $cmakeOptions -DCMAKE_INSTALL_PREFIX="$installDir"

Write-Host "Building and installing the project (first build)..."
cmake --build $buildDir --target install --config Debug

Write-Host "Building and installing the project (second build)..."
cmake --build $buildDir --target install --config Debug

Write-Host "Switching to installed directory and running tests..."
Set-Location $installDir
if (Test-Path .\ember_test -PathType Leaf) {
    Write-Host "Running installed test executable..."
    .\ember_test
} else {
    Write-Error "Error: Installed test executable not found in $installDir. Aborting."
    exit 1
}
