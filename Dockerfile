# syntax=docker/dockerfile:experimental

# Using Ubuntu 19.10 for now
FROM ubuntu:eoan AS builder
LABEL description="Development build environment"

# Update the distro and install our tools
RUN apt-get -y update && apt-get install -y \
 && apt-get -y install clang \
 && apt-get -y install cmake \
 && apt-get -y install git \
# dragging libstdc++-9 in for std::filesystem
 && apt-get -y install libstdc++-9-dev \
 && apt-get -y install software-properties-common \
 && add-apt-repository -y ppa:team-xbmc/ppa \
# Install required library packages
 && apt-get install -y libbotan-2-dev \
 && apt-get install -y libboost-program-options1.67-dev \
 && apt-get install -y libboost-locale1.67-dev \
 && apt-get install -y libboost-system1.67-dev \
 && apt-get install -y libmysqlcppconn-dev \
 && apt-get install -y zlib1g-dev \
 && apt-get install -y libpcre3-dev \
# Pulled from XBMC PPA
 && apt-get install -y flatbuffers-dev

# Copy source
ARG working_dir=/usr/src/ember
COPY . ${working_dir}
WORKDIR ${working_dir}
RUN update-alternatives --install /usr/bin/cc cc /usr/bin/clang 100 \
    && update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++ 100
# CMake arguments
# These can be overriden by passing them through to `docker build`
ARG build_optional_tools=1
ARG pcre_static_lib=1
ARG disable_threads=0
ARG build_type=Rel
ARG install_dir=/usr/local/bin

# Generate Makefile & compile
RUN --mount=type=cache,target=build \
    cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=${build_type} \
    -DCMAKE_INSTALL_PREFIX=${install_dir} \
    -DBUILD_OPT_TOOLS=${build_optional_tools} \
    -DPCRE_STATIC_LIB=${pcre_static_lib} \
    -DDISABLE_EMBER_THREADS=${disable_threads} \
    -DBOTAN_ROOT_DIR=/usr/include/botan-2/ \
    -DBOTAN_LIBRARY=/usr/lib/x86_64-linux-gnu/libbotan-2.so \
    && cd build && make -j install && make test

FROM ubuntu:eoan AS run_environment
ARG install_dir=/usr/local/bin
ARG working_dir=/usr/src/ember
WORKDIR ${install_dir}
RUN apt-get -y update \
    && apt-get install -y libbotan-2-9 \
    && apt-get install -y libmysqlcppconn7v5 \
    && apt-get install -y mysql-client
COPY --from=builder ${install_dir} ${install_dir}
RUN cp configs/*.dist .
COPY ./sql ${install_dir}/sql
COPY ./scripts ${install_dir}
COPY ./dbcs ${install_dir}/dbcs
