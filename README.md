# 🔥 **Ember**

## *An experimental modular MMO server emulator.*

---

Ember is an educational and research emulator developed to investigate MMO server architectures and bleeding-edge C++ language features and tooling.

While most emulators strive for feature completeness, Ember aims to be a production quality codebase and deployment architecture.

### Docker Quick Start

Ember uses Docker to make it easy to get the project up and running within minutes. Once you have Docker (version 19 and up) installed, simply run...


**Docker 19:**

Linux & MacOS:

```bash
DOCKER_BUILDKIT=1 docker build <path to Dockerfile>
```

Windows:

```cmd
set "DOCKER_BUILDKIT=1" && docker build <path to Dockerfile>
```

Ember uses `DOCKER_BUILDKIT=1` to enable experimental features in Docker 19 that allow for build caching. It can be omitted by setting it as an environmental variable.

**Docker 20+:**
```
docker build <path to Dockerfile>
```

Want to do it the traditional way? That's fine too, just see docs/GettingStarted.md.

### Need help?

We have a Discord server over at [https://discord.gg/WpPJzQS](https://discord.gg/WpPJzQS) or you can check [our website](https://emberemu.com) out for further documentation.

### Supported platforms

Ember aims to support the following platforms as a minimum:

| Operating System  | Architectures |
| :------------ |:---------------:|
| Linux         | x86, x64, ARMv7 |
| Windows       | x86, x64        |
| Mac OS        | x86, x64        |

### Compiler support

Any compiler version equal or greater than the supported version should be capable of compiling Ember. Minimum versions support all language features required but will not receive any fixes to support their use (e.g. compiler-specific workarounds).

|       |  Supported  |   Minimum   |
|-------|:-----------:|:-----------:|
| MSVC  | 19.30 (VS2022) | 19.30 (VS2022) |
| Clang |     17     |     17     |
| GCC   |     13     |     13     |

### Language support

Ember currently targets C++23 but allows for the use of upcoming language additions (e.g. technical specifications and drafts) as long as all three supported compilers provide a reasonable level of support.

### Build status

|  | master  | development |
| :------------ |:---------------:|:---------------:|
| AppVeyor | [![Build status](https://ci.appveyor.com/api/projects/status/wtctwhykqeelwk4g/branch/master?svg=true)](https://ci.appveyor.com/project/Chaosvex/ember/branch/master) | [![Build status](https://ci.appveyor.com/api/projects/status/wtctwhykqeelwk4g/branch/development?svg=true)](https://ci.appveyor.com/project/Chaosvex/ember/branch/development)  |
| Coverity | [![Coverity Scan Status](https://scan.coverity.com/projects/5653/badge.svg)](https://scan.coverity.com/projects/5653) | [![Coverity Scan Status](https://scan.coverity.com/projects/5653/badge.svg)](https://scan.coverity.com/projects/5653) |

### Why WoW 1.12.1?

Our primary goal isn't to produce a feature-complete, up-to-date emulator to use with newer clients. The 1.12.1 protocol was chosen as a fixed target to leverage the extensive research of prior projects, allowing for greater focus on writing code over reverse engineering.

### License

This project is licensed under the Mozilla Public License Version 2.0. A full copy of the license can be found in LICENSE or [by clicking here](http://mozilla.org/MPL/2.0/).
