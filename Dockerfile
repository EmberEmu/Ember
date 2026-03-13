# syntax=docker/dockerfile:experimental

FROM ubuntu:resolute AS builder
LABEL description="Development build environment"

ARG SKIP_UPGRADE
ARG USE_CLANG

# Update the distro and install our tools
RUN apt-get -y update

RUN if [ -n "$SKIP_UPGRADE" ]; then apt-get -y upgrade; fi

RUN apt-get -y install software-properties-common \
 && apt-get -y install wget \
 && apt-get -y install cmake \
 && apt-get -y install git \
 # Install required library packages
 && apt-get install -y libbotan-3-dev \
 && apt-get install -y libmysqlcppconn-dev \
 && apt-get install -y zlib1g-dev \
 && apt-get install -y libpcre3-dev \
 && apt-get install -y libflatbuffers-dev \
 && apt-get install -y ccache \
 && apt-get install -y ninja-build

RUN if [ -n "$USE_CLANG" ]; then                                        \
 apt-get -y install clang;                                              \
else                                                                    \
 apt-get -y install gcc-15 g++-15                                       \
 && update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-15 100    \
 && update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-15 100  \
 && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-15 100  \
 && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-15 100; \
fi

RUN wget -q https://archives.boost.io/release/1.90.0/source/boost_1_90_0.tar.gz \
 && tar -zxf boost_1_90_0.tar.gz \
 && cd boost_1_90_0 \
 && ./bootstrap.sh --with-libraries=system,program_options,headers \
 && ./b2 link=static install -d0 -j $(nproc) cxxflags="-std=c++23"

# Copy source
ARG working_dir=/usr/src/ember
COPY . ${working_dir}
WORKDIR ${working_dir}

ENV CCACHE_COMPILERCHECK=content
ENV CCACHE_BASEDIR=${working_dir}
ENV CCACHE_DIR=${working_dir}/build/.ccache

ENV CCACHE_LOGFILE=/tmp/ccache.log
ENV CCACHE_LOGLEVEL=debug
ENV CCACHE_IGNOREOPTION="-fmodules-ts -fmodule-mapper=*"
# CMake arguments
# These can be overriden by passing them through to `docker build`
ARG build_optional_tools=1
ARG build_type=Rel
ARG install_dir=/usr/local/bin

# Generate Makefile & compile
RUN --mount=type=cache,id=build-cache,target=/usr/src/ember/build \
    cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DCMAKE_BUILD_TYPE=${build_type}          \
    -DCMAKE_INSTALL_PREFIX=${install_dir}     \
    -DBUILD_OPT_TOOLS=${build_optional_tools} \
	&& ccache --max-size=10G                  \
    && cmake --build build -j$(nproc)         \
	&& cmake --install build                  \
    && ctest --test-dir build                 \
    && cat /tmp/ccache.log

FROM ubuntu:resolute AS run_environment
ARG install_dir=/usr/local/bin
ARG working_dir=/usr/src/ember
WORKDIR ${install_dir}
RUN apt-get -y update \
 && apt-get install -y libbotan-3-10 \
 && apt-get install -y libmysqlcppconn7v5 \
 && apt-get install -y mysql-client
COPY --from=builder ${install_dir} ${install_dir}
RUN cp configs/*.dist .
COPY ./sql ${install_dir}/sql
COPY ./scripts ${install_dir}
COPY ./dbcs ${install_dir}/dbcs