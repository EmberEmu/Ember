# Copyright (c) 2025 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

$buildDir            = "build"
$installDir          = ".\build\bin"
$buildType           = "Debug"

$buildOptionalTools  = "-1"
$disableEmberThreads = "0"

####################################################
# Install dependencies through conan
####################################################
# Use pip for getting conan
Write-Host "Installing Conan..."
pip install conan --user
$ConanScripts = "$env:APPDATA\Python\Python39\Scripts"
Write-Host "Adding $ConanScripts to PATH"
$env:PATH += ";$ConanScripts"

# Detect and patch the default profile for Release and C++23
# These values need to be hardcoded, so change to build_type=Release for a release build
conan profile detect
$profilePath = (& conan profile path default).Trim()
(Get-Content $profilePath) `
    -replace '^(build_type=).*$', 'build_type=Debug' `
    -replace '^(compiler\.cppstd=).*$', 'compiler.cppstd=23' `
    | Set-Content $profilePath -Force
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

# Dependencies to install via Conan.
$conanCmd = "conan install"
$conanCmd += " --requires boost/1.87.0"
$conanCmd += " --requires botan/3.6.1"
$conanCmd += " --requires flatbuffers/24.12.23"
$conanCmd += " --requires pcre/8.45"
$conanCmd += " --requires zlib/1.3.1"
$conanCmd += " -of $buildDir --build missing -g CMakeToolchain -g CMakeDeps --profile default"

Write-Host "Running Conan install..."
Invoke-Expression $conanCmd

####################################################
# --- Patch Conan configuration ---
####################################################
# Define the top-level CMakeLists.txt path (assumed to be in the project root)
$cmakeFile = "CMakeLists.txt"

# Read the entire file into a single string (assuming the file exists)
$cmakeContent = Get-Content $cmakeFile -Raw

# --- Patch the Botan block ---
# We match from "if(TARGET Botan::Botan-static)" and ends with the first "endif()"
$botanRegex = '(?ms)if\s*\(TARGET\s+Botan::Botan-static\).*?endif\(\)'
$newBotanBlock = @"
if(TARGET botan::botan-static)
  set(BOTAN_LIBRARY botan::botan-static)
  message(STATUS "Using Botan static library")
elseif(TARGET botan::botan)
  set(BOTAN_LIBRARY botan::botan)
  message(STATUS "Using Botan shared library")
else()
  message(FATAL_ERROR "No valid Botan target found")
endif()
"@
$cmakeContent = [regex]::Replace($cmakeContent, $botanRegex, $newBotanBlock, 
                                 [System.Text.RegularExpressions.RegexOptions]::Singleline)

# --- Patch the PCRE block ---
# Find the PCRE find_package line and then append a line setting PCRE_LIBRARY to pcre::pcre.
$pcrePattern = "(find_package\(PCRE\s+8\.39\s+REQUIRED\))"
$pcreReplacement = '$1' + "`nset(PCRE_LIBRARY pcre::pcre)"
$cmakeContent = [regex]::Replace($cmakeContent, $pcrePattern, $pcreReplacement)

# Write the modified content back to the CMakeLists.txt
Set-Content -Path $cmakeFile -Value $cmakeContent
Write-Host "Updated top-level CMakeLists.txt successfully."

####################################################
# Install MySQL Connector/C++ 
# (Prebuilt for x86_64 / Source-compile for ARM64)
####################################################
if (-not (Test-Path "C:\mysql-connector-c++\")) {
    # Define a local cache directory for downloads (relative to this script)
    $cacheDir = "tmp"
    if (-not (Test-Path $cacheDir)) {
        New-Item -ItemType Directory -Path $cacheDir | Out-Null
    }

    # Define the cache ZIP file path for MySQL Connector
    $CACHE_ZIP = Join-Path $cacheDir "mysql-connector.zip"

    # Determine architecture and set download parameters accordingly
    if ($env:PROCESSOR_ARCHITECTURE -eq "ARM64") {
        Write-Host "ARM64 architecture detected. Downloading MySQL Connector/C++ source code."
        $url = "https://github.com/mysql/mysql-connector-cpp/archive/refs/tags/9.3.0.zip"
        $connectorSubDir = "mysql-connector-c++-9.3.0"
    } 
    else {
        Write-Host "x86_64 architecture detected. Using prebuilt MySQL Connector/C++ binaries."
        $url = "https://dev.mysql.com/get/Downloads/Connector-C++/mysql-connector-c++-9.3.0-winx64-debug.zip"
        $connectorSubDir = "mysql-connector-c++-9.3.0-winx64-debug"
    }

    Write-Host "Downloading MySQL Connector/C++ from $url"
    if (Get-Command curl -ErrorAction SilentlyContinue) {
        Write-Host "Downloading using curl..."
        curl -L $url -o $CACHE_ZIP
    } 
    elseif (Get-Command wget -ErrorAction SilentlyContinue) {
        Write-Host "Downloading using wget..."
        wget $url -O $CACHE_ZIP
    } 
    else {
        Write-Host "Downloading using Invoke-WebRequest..."
        Invoke-WebRequest -Uri $url -OutFile $CACHE_ZIP
    }

    # Set extraction directory for the connector (within the cache folder)
    $extractDir = Join-Path $cacheDir "mysql-connector-c++"
    New-Item -ItemType Directory -Path $extractDir | Out-Null

    Write-Host "Extracting MySQL Connector/C++..."
    Expand-Archive -Path $CACHE_ZIP -DestinationPath $extractDir

    # Determine source base directory (if the ZIP extract creates a subfolder)
    $sourceBase = Join-Path $extractDir $connectorSubDir
    if (-not (Test-Path $sourceBase)) {
        $sourceBase = $extractDir
    }

    if ($env:PROCESSOR_ARCHITECTURE -eq "ARM64") {
        # For ARM64: Downloaded Source – Build Required
        Write-Host "ARM64 architecture: MySQL Connector/C++ source downloaded."
        Write-Host "Source code is available at: $sourceBase"
        Write-Host "You must now build the connector from source for ARM64."
        Write-Host "For example, use CMake along with your preferred build configuration."
    } 
    else {
        # For x86_64: Install Prebuilt Libraries
        $mysqlconcppTargetDir = "C:\mysql-connector-c++"

        if (-not (Test-Path $mysqlconcppTargetDir)) {
            New-Item -ItemType Directory -Path $mysqlconcppTargetDir | Out-Null
        }

        Write-Host "Installing MySQL Connector/C++ (prebuilt x86_64) to $mysqlconcppTargetDir..."

        # Set the expected extracted folder name from the ZIP archive (as provided by Oracle)
        $sourceBase = Join-Path $extractDir "mysql-connector-c++-9.3.0-winx64"

        # Instead of selecting only a few subdirectories, copy the entire contents to preserve the layout.
        Copy-Item -Path (Join-Path $sourceBase "*") -Destination $mysqlconcppTargetDir -Recurse -Force

        Write-Host "MySQL Connector/C++ installed at: $mysqlconcppTargetDir"
    }
} 
else {
    Write-Host "MySQL Connector/C++ is already installed at $mysqlconcppTargetDir"
}

$env:CMAKE_PREFIX_PATH = "$mysqlconcppTargetDir;$env:CMAKE_PREFIX_PATH"
Write-Host "CMAKE_PREFIX_PATH set to: $env:CMAKE_PREFIX_PATH"

####################################################
# Configure and Build Ember
####################################################
Write-Host "=== Configuring project with CMake ==="

cmake -S . -B $buildDir -G "Visual Studio 17 2022" `
      -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>" `
      -DCMAKE_TOOLCHAIN_FILE="$buildDir\conan_toolchain.cmake" `
      -DBUILD_OPT_TOOLS="$buildOptionalTools" `
      -DDISABLE_EMBER_THREADS="$disableEmberThreads" `
      -DCMAKE_INSTALL_PREFIX="$installDir"

Write-Host "Building and installing the project..."
cmake --build $buildDir --target install --config "$buildType"

####################################################
# Run the unit_tests for regression control
####################################################
Write-Host "=== Switching to installed directory and running tests ==="
Set-Location $installDir
if (Test-Path ".\unit_tests.exe") {
    .\unit_tests.exe
} 
else {
    Write-Host "Warning: Installed test executable not found."
}

Write-Host "=== Build, install, and test complete ==="
