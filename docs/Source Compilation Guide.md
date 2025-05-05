# 🔥 **EMBER** DOCUMENTATION: _COMPILING EMBER USING CLI_

Open a terminal at the directory you want to fetch Ember (/and vcpkg) into.

<details>
  <summary>
    Windows
    <img src="https://upload.wikimedia.org/wikipedia/commons/8/87/Windows_logo_-_2021.svg" alt="Windows Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
  </summary>

> This guide is going to be assuming the C: drive but you can pick any folder;
>
> ```shell
> cd C:\
> ```
> 
> _This guide is assuming you are using powershell. 
> You can use cmd, but you need to amend the syntax of the multi line commands_

</details>

<details>
  <summary>
    macOS
    <img src="https://upload.wikimedia.org/wikipedia/commons/a/ab/Icon-Mac.svg" alt="Apple Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
    <span style="margin: 0 4px;">/</span>
    Linux
    <img src="https://upload.wikimedia.org/wikipedia/commons/a/af/Tux.png" alt="Linux (Tux) Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">

  </summary>

> This guide is going to be assuming the $HOME directory but you can pick any directory;
>
> ```bash
> cd $HOME
> ```

</details>

## **1. PRE-REQUIREMENTS:**

In order to compile Ember from source some build tools are needed. 
In particular we need git and cmake for this, as well as anything platform dependant.

<details>
  <summary>
    Windows
    <img src="https://upload.wikimedia.org/wikipedia/commons/8/87/Windows_logo_-_2021.svg" alt="Windows Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
  </summary>

> ### **Install build dependencies**
> 
> ```shell
> winget install --id=Git.Git -e --silent --accept-package-agreements --accept-source-agreements; `
> winget install --id=Kitware.CMake -e --silent --accept-package-agreements --accept-source-agreements; `
> winget install --id=Ninja-build.Ninja -e --silent --accept-package-agreements --accept-source-agreements
> ```
> **_Alternatively you can remove all the silent agreements and agree manually in the terminal as well as any popups._**

</details>

<details>
  <summary>
    macOS
    <img src="https://upload.wikimedia.org/wikipedia/commons/a/ab/Icon-Mac.svg" alt="Apple Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
  </summary>

> ### **MacOS pre-requirements**
>
> **- Install xcode**
> ```zsh
> xcode-select --install
> ```
> **- Install homebrew**
> ```zsh
> /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
> ```
> ```zsh
> brew update && \
> brew upgrade
> ```
> 
> ### **Install build dependencies**
> ```zsh
> brew install git cmake make ninja curl
> ```
> **_cmake can be added permanently to the path by amending the .zshrc file in $HOME to include this line_**
> ```zsh
> export PATH=/Applications/CMake.app/Contents/bin:$PATH
> ```
> And then you can load the changes by running:
> ```zsh
> source ~/.zshrc
> ```

</details>

<details>
  <summary>
    Linux
    <img src="https://upload.wikimedia.org/wikipedia/commons/a/af/Tux.png" alt="Linux (Tux) Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
  </summary>

> ### **Install build dependencies**
> ```bash
> apt-get -y update && apt-get -y upgrade && \
> apt-get -y install software-properties-common && \
> apt-get -y install git cmake make wget tar
> ```

</details>

Next we are going to acquire one (or several) of the compilers which we are going to use for compiling Ember;

<details>
  <summary>
    Windows
    <img src="https://upload.wikimedia.org/wikipedia/commons/8/87/Windows_logo_-_2021.svg" alt="Windows Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
  </summary>

> ### **- Install visual studio build tools for msvc**
> 
> ```shell
> winget install --id=Microsoft.VisualStudio.2022.BuildTools -e --silent --accept-package-agreements --accept-source-agreements --override `
>   "--add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.Windows10SDK `
>    --add Microsoft.VisualStudio.Component.VC.Redist.MSM --includeRecommended"; `
> & "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
> ```
> **Follow the on-screen install instructions for visual studio, 
> _- Additionally you can choose to include clang as an optional compiler by selecting it in individual packages_**

</details>

<details>
  <summary>
    macOS
    <img src="https://upload.wikimedia.org/wikipedia/commons/a/ab/Icon-Mac.svg" alt="Apple Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
  </summary>

> ### **- Install either llvm for libc++ or gcc for libstdc++**
> 
> **a) _Install llvm for libc++_**
> ```zsh
> brew install llvm
> ```
> **b) _Install gcc for libstdc++_**
> ```zsh
> brew install gcc
> ```

</details>

<details>
  <summary>
    Linux
    <img src="https://upload.wikimedia.org/wikipedia/commons/a/af/Tux.png" alt="Linux (Tux) Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
  </summary>

> ### **- Install either llvm for libc++ or gcc for libstdc++**
> 
> **a) _Install llvm for libc++_**
> ```bash
> apt-get -y install llvm && \
> apt-get -y install llvm-dev
> ```
> **b) _Install gcc for libstdc++_**
> ```bash
> apt-get -y install build-essential && \
> apt-get -y install libstdc++-dev
> ```

</details>


## 2. GETTING DEPENDENCIES:

Ember also needs a few libraries in order to compile, either manually or through vcpkg;

### A) Install using vcpkg

<details>
  <summary>
    Windows
    <img src="https://upload.wikimedia.org/wikipedia/commons/8/87/Windows_logo_-_2021.svg" alt="Windows Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
  </summary>

> ### **- Download and install vcpkg:**
> ```shell
> git clone https://github.com/microsoft/vcpkg.git; `
> .\vcpkg\bootstrap-vcpkg.bat; `
> .\vcpkg\vcpkg integrate install
> ```

</details>

<details>
  <summary>
    macOS
    <img src="https://upload.wikimedia.org/wikipedia/commons/a/ab/Icon-Mac.svg" alt="Apple Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
  </summary>

> ### - **Download and deploy vcpkg:**
> ```zsh
> git clone https://github.com/microsoft/vcpkg.git && \
> ./vcpkg/bootstrap-vcpkg.sh && \
> ./vcpkg/vcpkg integrate install
> ```
> **_vcpkg can be added permanently to the path by amending the .zshrc file in $HOME to include these lines_**
> ```zsh
> export VCPKG_ROOT=$HOME/vcpkg
> export PATH=$VCPKG_ROOT:$PATH
> ```
> And then you can load the changes by running:
> ```zsh
> source ~/.zshrc
> ```

</details>

<details>
  <summary>
    Linux
    <img src="https://upload.wikimedia.org/wikipedia/commons/a/af/Tux.png" alt="Linux (Tux) Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
  </summary>

> ### - **Download and deploy vcpkg:**
> ```bash
> git clone https://github.com/microsoft/vcpkg.git && \
> ./vcpkg/bootstrap-vcpkg.sh && \
> ./vcpkg/vcpkg integrate install
> ```

</details>

### B) Install using native tools and sources/precompiled binaries

<details>
  <summary>
    Windows
    <img src="https://upload.wikimedia.org/wikipedia/commons/8/87/Windows_logo_-_2021.svg" alt="Windows Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
  </summary>

> ### **- Steps incoming...**
> ```shell
> ```

</details>

<details>
  <summary>
    macOS
    <img src="https://upload.wikimedia.org/wikipedia/commons/a/ab/Icon-Mac.svg" alt="Apple Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
  </summary>

> **Packages through homebrew**
> ```zsh
> brew install boost && \
> brew install botan && \
> brew install mysql-client && \
> brew install openssl && \
> brew install flatbuffers && \
> brew install pcre && \
> brew install zlib
> ```
> **Install MySQL Connector/C++**
> ```zsh
> wget https://dev.mysql.com/get/Downloads/Connector-C++/mysql-connector-c++-9.3.0-macos15-arm64.tar.gz -O  && \
> mkdir -p /usr/local/lib/cmake/mysql-concpp  && \
> tar -zxf "mysql-connector-c++-9.3.0-macos15-arm64.tar.gz" -C /usr/local/lib/cmake/mysql-concpp --strip-components=1
> ```
> **Installing MySQL Connector/C++ headers and libraries.**
> ```zsh
> mkdir -p /usr/local/include/mysql-concpp
> cp -r /usr/local/lib/cmake/mysql-concpp/include/. /usr/local/include/mysql-concpp/
> cp -r /usr/local/lib/cmake/mysql-concpp/lib64/. /usr/local/lib/
> ```

</details>

<details>
  <summary>
    Linux
    <img src="https://upload.wikimedia.org/wikipedia/commons/a/af/Tux.png" alt="Linux (Tux) Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
  </summary>

> **Packages from apt-get**
> ```bash
> apt-get install -y libbotan-3-dev && \
> apt-get install -y libmysqlclient-dev && \
> apt-get install -y libssl-dev && \
> apt-get install -y zlib1g-dev && \
> apt-get install -y libpcre3-dev && \
> apt-get install -y libflatbuffers-dev
> ```
> **Boost:**
> ```bash
> wget -q https://archives.boost.io/release/1.87.0/source/boost_1_87_0.tar.gz && \
> tar -zxf boost_1_87_0.tar.gz && \
> ./boost_1_87_0/bootstrap.sh --with-libraries=system,program_options,headers && \
> ./boost_1_87_0/b2 link=static install -d0 -j $(nproc) cxxflags="-std=c++23"
> ```
> **Mysql-connector-c++:**
> ```bash
> arch=$(uname -m) && case "$arch" in \
>   x86_64) url="https://dev.mysql.com/get/Downloads/Connector-C++/mysql-connector-c++-9.3.0-linux-glibc2.28-x86-64bit.tar.gz" ;; \
>   aarch64) url="https://dev.mysql.com/get/Downloads/Connector-C++/mysql-connector-c++-9.3.0-linux-glibc2.28-aarch64.tar.gz" ;; \
>   *) echo "Unsupported architecture: $arch" && exit 1 ;; esac && \
> echo "Downloading MySQL Connector/C++ from ${url}" && \
> wget "$url" -O /tmp/mysql-connector.tar.gz && \
> mkdir -p /usr/lib/cmake/mysql-concpp && \
> tar -zxf /tmp/mysql-connector.tar.gz -C /usr/lib/cmake/mysql-concpp --strip-components=1 && \
> echo "Installing headers and libraries..." && \
> mkdir -p /usr/include/mysql-cppconn && \
> cp -r /usr/lib/cmake/mysql-concpp/include/* /usr/include/mysql-cppconn/ && \
> cp -r /usr/lib/cmake/mysql-concpp/lib64/* /usr/local/lib/ && \
> ldconfig && \
> echo "MySQL Connector/C++ installed."
>  ```
> **Patch the Botan config file for header paths: (THIS WILL BE PATCHED IN BOTAN 3.8!)**
> ```bash
> sh -c 'arch=$(dpkg-architecture -qDEB_HOST_MULTIARCH) && \
>   BOTAN_CFG=$(find / -type f -iname "botan-config.cmake" 2>/dev/null | grep "/usr/lib/$arch" | head -n1) && \
>   [ -n "$BOTAN_CFG" ] && \
>   sed -i -e "s|\${_Botan_PREFIX}/include|/usr/include|g" -e "s|\${_Botan_PREFIX}/lib|/usr/lib/$arch|g" "$BOTAN_CFG"'
> ```

</details>


## 3. DOWNLOAD/UPDATE EMBER:

We are using git for both downloading and updating Ember

### First time setting up Ember:

<details>
  <summary>
    Windows
    <img src="https://upload.wikimedia.org/wikipedia/commons/8/87/Windows_logo_-_2021.svg" alt="Windows Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
    <span style="margin: 0 4px;">/</span>
    macOS
    <img src="https://upload.wikimedia.org/wikipedia/commons/a/ab/Icon-Mac.svg" alt="Apple Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
    <span style="margin: 0 4px;">/</span>
    Linux
    <img src="https://upload.wikimedia.org/wikipedia/commons/a/af/Tux.png" alt="Linux (Tux) Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
  </summary>

> Make sure you are in the directory where you want Ember to reside, then run the following git command:
> ```shell
> git clone https://github.com/EmberEmu/Ember.git
> ```
> Change the current directory to Ember for building:
> ```shell
> cd Ember
> ```

</details>

### Updating Ember:

<details>
  <summary>
    Windows
    <img src="https://upload.wikimedia.org/wikipedia/commons/8/87/Windows_logo_-_2021.svg" alt="Windows Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
    <span style="margin: 0 4px;">/</span>
    macOS
    <img src="https://upload.wikimedia.org/wikipedia/commons/a/ab/Icon-Mac.svg" alt="Apple Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
    <span style="margin: 0 4px;">/</span>
    Linux
    <img src="https://upload.wikimedia.org/wikipedia/commons/a/af/Tux.png" alt="Linux (Tux) Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
  </summary>

> Open a terminal at the directory where Ember is cloned to and run the following git commands:
> ```shell
> git fetch origin
> ```
> This fetches the changes on the remote
> ```shell
> git rebase origin/HEAD
> ```
> And this applies it to your downloaded Ember repository. 
> This is assuming you've made no changes to the source code, 
> otherwise you have to stash or commit your changes before doing the rebase or it'll throw an error.

</details>

## 4. BUILD EMBER WITH CMAKE:

We are now ready to compile Ember using cmake and the compiler of your choice;

<details>
  <summary>
    Windows
    <img src="https://upload.wikimedia.org/wikipedia/commons/8/87/Windows_logo_-_2021.svg" alt="Windows Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
  </summary>

> ## Build with cmake and msvc using visual studio build tools:
> 
> ```shell
> cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
>   -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake" `
>   -DVCPKG_TARGET_TRIPLET=x64-windows-static `
>   -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>" `
>   -DCMAKE_INSTALL_PREFIX="\build\bin"
> ```
> Then run the cmake --build, remember to configure either for release or debug:
> **Run this twice or it might failt to find the generated includes**
> ```shell
> cmake --build build --target install --config Debug
> ```

</details>

<details>
  <summary>
    macOS
    <img src="https://upload.wikimedia.org/wikipedia/commons/a/ab/Icon-Mac.svg" alt="Apple Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
  </summary>

> ### **- cmake with either llvm for libc++ or gcc for libstdc++**
> 
> **a) llvm with libc++**
> ```zsh
> cmake -S . -B build \
>   -DCMAKE_TOOLCHAIN_FILE="$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake" \
>   -DCMAKE_C_COMPILER="/opt/homebrew/opt/llvm/bin/clang" \
>   -DCMAKE_CXX_COMPILER="/opt/homebrew/opt/llvm/bin/clang++" \
>   -DCMAKE_INSTALL_PREFIX="/build/bin"
> ```
> **b) gcc with libstdc++**
> ```zsh
> cmake -S . -B build \
>   -DCMAKE_TOOLCHAIN_FILE="$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake" \
>   -DCMAKE_C_COMPILER="/opt/homebrew/bin/gcc-14" \
>   -DCMAKE_CXX_COMPILER="/opt/homebrew/bin/g++-14" \
>   -DCMAKE_INSTALL_PREFIX="/build/bin"
> ```
> Then run the cmake --build, remember to configure either for release or debug:
> ```zsh
> cmake --build build --target install --config Debug
> ```

</details>

<details>
  <summary>
    Linux
    <img src="https://upload.wikimedia.org/wikipedia/commons/a/af/Tux.png" alt="Linux (Tux) Logo" width="20" height="20" style="vertical-align: middle; margin-right: 4px;">
  </summary>

> ### Generate Makefile & compile
>
> ```bash
> cmake -S . -B build \
>   -DCMAKE_TOOLCHAIN_FILE="$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake" \
>   -DCMAKE_INSTALL_PREFIX="/build/bin"
> ```
> Then run the cmake --build, remember to configure either for release or debug:
> ```bash
> cmake --build build --target install --config Debug
> ```

</details>