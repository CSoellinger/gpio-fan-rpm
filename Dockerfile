# Multi-architecture build container for gpio-fan-rpm
ARG PKG_TAG=1.0.0
FROM debian:bookworm-slim AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    autoconf \
    automake \
    libtool \
    pkg-config \
    autoconf-archive \
    linux-headers-generic || true \
    && rm -rf /var/lib/apt/lists/*

# Build and install libgpiod v2 from source
# Debian Bookworm only has v1.6, but we need v2.x
RUN git clone --depth 1 --branch v2.1.3 https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git /tmp/libgpiod && \
    cd /tmp/libgpiod && \
    ./autogen.sh --enable-tools=yes --prefix=/usr && \
    make -j$(nproc) && \
    make install && \
    ldconfig && \
    cd / && \
    rm -rf /tmp/libgpiod

# Set working directory
WORKDIR /build

# Copy source files
COPY . /build/

# Build
ARG PKG_TAG=1.0.0
RUN mkdir -p build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DPKG_TAG=${PKG_TAG} .. && \
    make -j$(nproc) && \
    strip gpio-fan-rpm

# Runtime stage - minimal image
FROM debian:bookworm-slim

RUN rm -rf /var/lib/apt/lists/*

# Copy libgpiod v2 from builder (installed to /usr/lib)
COPY --from=builder /usr/lib/libgpiod.so* /usr/lib/
RUN ldconfig

# Copy binary from builder
COPY --from=builder /build/build/gpio-fan-rpm /usr/local/bin/

# Set entrypoint
ENTRYPOINT ["/usr/local/bin/gpio-fan-rpm"]
CMD ["--help"]
