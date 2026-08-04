FROM docker.io/library/ubuntu:24.04

ARG A30_TOOLCHAIN_URL=https://github.com/steward-fu/website/releases/download/miyoo-a30/a30_toolchain-v1.0.tar.gz

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    autoconf \
    automake \
    build-essential \
    ca-certificates \
    cmake \
    file \
    gettext \
    git \
    libtool \
    m4 \
    make \
    ninja-build \
    pkg-config \
    python3 \
    unzip \
    wget \
    xz-utils \
    && rm -rf /var/lib/apt/lists/*

RUN wget -qO /tmp/a30-toolchain.tar.gz "${A30_TOOLCHAIN_URL}" \
    && tar xzf /tmp/a30-toolchain.tar.gz -C /opt \
    && rm /tmp/a30-toolchain.tar.gz

ENV CROSS_TRIPLE=arm-a30-linux-gnueabihf
ENV CROSS_ROOT=/opt/a30
ENV SYSROOT=/opt/a30/arm-a30-linux-gnueabihf/sysroot
ENV PREFIX=/opt/a30/arm-a30-linux-gnueabihf/sysroot/usr
ENV PREFIX_LOCAL=/opt/nextui
ENV ARCH=arm
ENV UNION_PLATFORM=my282

# The archive contains the real Buildroot compiler binaries but not usable
# wrapper scripts. Always pass the A30 sysroot explicitly.
RUN rm -f ${CROSS_ROOT}/bin/${CROSS_TRIPLE}-gcc ${CROSS_ROOT}/bin/${CROSS_TRIPLE}-g++ \
    && printf '#!/bin/sh\nexec /opt/a30/bin/arm-a30-linux-gnueabihf-gcc-13.2.0.br_real --sysroot=/opt/a30/arm-a30-linux-gnueabihf/sysroot "$@"\n' > ${CROSS_ROOT}/bin/${CROSS_TRIPLE}-gcc \
    && printf '#!/bin/sh\nexec /opt/a30/bin/arm-a30-linux-gnueabihf-c++.br_real --sysroot=/opt/a30/arm-a30-linux-gnueabihf/sysroot "$@"\n' > ${CROSS_ROOT}/bin/${CROSS_TRIPLE}-g++ \
    && chmod +x ${CROSS_ROOT}/bin/${CROSS_TRIPLE}-gcc ${CROSS_ROOT}/bin/${CROSS_TRIPLE}-g++

# Restore soname and development links omitted from the published archive.
RUN cd ${SYSROOT}/lib \
    && ln -sf ld-2.23.so ld-linux-armhf.so.3 \
    && ln -sf libc-2.23.so libc.so.6 \
    && ln -sf libdl-2.23.so libdl.so.2 \
    && ln -sf libm-2.23.so libm.so.6 \
    && ln -sf libpthread-2.23.so libpthread.so.0 \
    && ln -sf libresolv-2.23.so libresolv.so.2 \
    && ln -sf librt-2.23.so librt.so.1 \
    && cd ${SYSROOT}/usr/lib \
    && ln -sf libSDL2-2.0.so.0.2800.5 libSDL2.so \
    && ln -sf libMali.so libEGL.so \
    && ln -sf libMali.so libGLESv2.so \
    && ln -sf libasound.so.2.0.0 libasound.so \
    && ln -sf libfreetype.so.6.20.1 libfreetype.so

ENV PATH=${CROSS_ROOT}/bin:${PATH}
ENV CROSS_COMPILE=${CROSS_TRIPLE}-
ENV CC=${CROSS_TRIPLE}-gcc
ENV CXX=${CROSS_TRIPLE}-g++
ENV AR=${CROSS_TRIPLE}-ar
ENV AS=${CROSS_TRIPLE}-as
ENV LD=${CROSS_TRIPLE}-ld
ENV STRIP=${CROSS_TRIPLE}-strip
ENV PKG_CONFIG_SYSROOT_DIR=${SYSROOT}
ENV PKG_CONFIG_LIBDIR=${PREFIX}/lib/pkgconfig
ENV PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig
ENV CMAKE_TOOLCHAIN_FILE=${CROSS_ROOT}/Toolchain.cmake

COPY toolchain-armv7.cmake ${CMAKE_TOOLCHAIN_FILE}

RUN mkdir -p ${PREFIX_LOCAL}/include ${PREFIX_LOCAL}/lib

# NextUI's shared audio path uses libsamplerate. The published A30 SDK does
# not include it, so build the same portable library that other NextUI targets
# package alongside MinArch.
RUN wget -qO /tmp/libsamplerate.tar.xz \
        https://github.com/libsndfile/libsamplerate/releases/download/0.2.2/libsamplerate-0.2.2.tar.xz \
    && tar xf /tmp/libsamplerate.tar.xz -C /tmp \
    && cd /tmp/libsamplerate-0.2.2 \
    && ./configure \
        --host=${CROSS_TRIPLE} \
        --prefix=${PREFIX_LOCAL} \
        --disable-static \
        --disable-fftw \
        --disable-sndfile \
    && make -j"$(nproc)" \
    && make install \
    && rm -rf /tmp/libsamplerate-0.2.2 /tmp/libsamplerate.tar.xz

# MinArch rewind support requires LZ4. Keep it in the port-local prefix so the
# stock A30 sysroot remains an accurate representation of device userspace.
RUN wget -qO /tmp/lz4.tar.gz \
        https://github.com/lz4/lz4/archive/refs/tags/v1.10.0.tar.gz \
    && tar xzf /tmp/lz4.tar.gz -C /tmp \
    && make -C /tmp/lz4-1.10.0/lib \
        CC=${CC} PREFIX=${PREFIX_LOCAL} BUILD_SHARED=yes -j"$(nproc)" \
    && make -C /tmp/lz4-1.10.0/lib \
        CC=${CC} PREFIX=${PREFIX_LOCAL} BUILD_SHARED=yes install \
    && rm -rf /tmp/lz4-1.10.0 /tmp/lz4.tar.gz

# Game Tracker and Battery use SQLite databases. The stock A30 firmware does
# not provide a 32-bit libsqlite3, so build and package a standalone copy.
ARG SQLITE_VERSION=3530400
ARG SQLITE_SHA3=454e45f61c6bd75b7420e7190732dea03ce6639c63ada47bbc592f67fc340338
RUN wget -qO /tmp/sqlite.tar.gz \
        https://www.sqlite.org/2026/sqlite-autoconf-${SQLITE_VERSION}.tar.gz \
    && python3 -c "import hashlib; p='/tmp/sqlite.tar.gz'; expected='${SQLITE_SHA3}'; actual=hashlib.sha3_256(open(p,'rb').read()).hexdigest(); assert actual == expected, (actual, expected)" \
    && tar xzf /tmp/sqlite.tar.gz -C /tmp \
    && cd /tmp/sqlite-autoconf-${SQLITE_VERSION} \
    && ./configure \
        --host=${CROSS_TRIPLE} \
        --prefix=${PREFIX_LOCAL} \
        --disable-static \
        --enable-shared \
        --disable-readline \
    && make -j"$(nproc)" \
    && make install \
    && rm -rf /tmp/sqlite-autoconf-${SQLITE_VERSION} /tmp/sqlite.tar.gz

VOLUME /root/workspace
WORKDIR /root/workspace
