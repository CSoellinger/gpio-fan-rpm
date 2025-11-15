# Building gpio-fan-rpm

This document describes how to build gpio-fan-rpm for different architectures.

## Quick Start

### Native Build (Local System)

If you have CMake and libgpiod development libraries installed:

```bash
# Using Makefile
make

# Or using build script
./build.sh local

# Or directly with CMake
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

### Container Build (Recommended)

Build using Docker or Podman (automatically detected):

```bash
# Build for current architecture
./build.sh container

# Or using Makefile
make container
```

### Cross-Compilation

Build for different architectures using containers:

```bash
# Build for aarch64 (ARM 64-bit, Cortex-A53)
./build.sh cross arm64
# or
make cross-arm64

# Build for x86_64
./build.sh cross amd64
# or
make cross-amd64

# Build for all supported architectures
./build.sh all
# or
make all-arch
```

Binaries will be placed in the `dist/` directory.

## Prerequisites

### For Local Builds (Linux only)

- CMake 3.10 or later
- C compiler (gcc or clang)
- libgpiod development libraries (libgpiod-dev on Debian/Ubuntu)
- pthread library (usually included with system)

On Debian/Ubuntu:
```bash
sudo apt-get install build-essential cmake libgpiod-dev
```

**Note for macOS users:** Native builds are not supported on macOS because libgpiod is Linux-specific. Use container builds instead (see below).

### For Container Builds

- Docker or Podman
- For multi-arch builds: docker buildx or podman with qemu-user-static

On Debian/Ubuntu:
```bash
# For Docker
sudo apt-get install docker.io docker-buildx

# For Podman
sudo apt-get install podman qemu-user-static
```

On macOS:
```bash
# Using Homebrew
brew install podman
# or
brew install docker
```

## Build Options

### Build Types

```bash
# Release build (optimized, default)
./build.sh --release local
make release

# Debug build (with debug symbols)
./build.sh --debug local
make debug
```

### Custom Version Tag

```bash
# Set custom version tag (shown in --version output)
./build.sh --tag v2.0.0 local
./build.sh --tag "$(git describe --tags)" cross arm64

# Using CMake directly
cmake -DCMAKE_BUILD_TYPE=Release -DPKG_TAG=v2.0.0 ..

# Using environment variable
PKG_TAG=v2.0.0 ./build.sh cross arm64
```

### Output Directory

```bash
# Specify custom output directory
./build.sh --output mydir cross arm64
# or
OUTPUT_DIR=mydir ./build.sh cross arm64
```

### Container Engine Selection

```bash
# Force use of podman
./build.sh --podman container

# Force use of docker
./build.sh --docker container

# Or use environment variable
CONTAINER_ENGINE=podman ./build.sh container
```

## Build System Files

- **CMakeLists.txt** - Main CMake build configuration
- **Makefile** - Convenient wrapper for common build tasks
- **build.sh** - Advanced build script with multi-arch support
- **Dockerfile** - Container image for native builds
- **Dockerfile.cross** - Container image for cross-compilation
- **cmake/toolchains/** - CMake toolchain files for cross-compilation

## Cross-Compilation (Advanced)

### Using CMake Toolchain Files

For cross-compilation on your local system (requires cross-compiler installed):

```bash
mkdir build-arm64 && cd build-arm64
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/aarch64-linux-gnu.cmake ..
make
```

This requires `aarch64-linux-gnu-gcc` cross-compiler installed:

```bash
# On Debian/Ubuntu
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
```

### Supported Architectures

| Architecture | CMake Platform | Binary Name |
|--------------|----------------|-------------|
| x86_64 / amd64 | linux/amd64 | gpio-fan-rpm-x86_64 |
| aarch64 / arm64 | linux/arm64 | gpio-fan-rpm-aarch64 |

The aarch64 build is optimized for Cortex-A53 processors.

## Cleaning

```bash
# Clean build artifacts
./build.sh clean
# or
make clean
```

## Installation

```bash
# Install to system (default: /usr/local/bin)
make install

# Or specify installation prefix
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make install
```

## Building on macOS

macOS does not support native builds because libgpiod is Linux-specific (it interfaces with Linux GPIO subsystem). However, you can build Linux binaries on macOS using containers.

### Prerequisites (macOS)

Install Docker or Podman:

```bash
# Using Homebrew
brew install podman
# or
brew install docker
```

### Recommended Build Commands (macOS)

```bash
# Build for your target architecture
make cross-arm64        # For Raspberry Pi, etc.
make cross-amd64        # For x86_64 Linux systems

# Build for all architectures
make all-arch

# Test container build
make container
```

If you try to run `make` or `./build.sh local` on macOS, you'll receive a helpful error message directing you to use container builds instead.

## Troubleshooting

### libgpiod not found

Ensure libgpiod development package is installed:
```bash
sudo apt-get install libgpiod-dev
```

### Cross-compilation fails with QEMU errors

Install QEMU user-mode emulation:
```bash
sudo apt-get install qemu-user-static
```

For Docker on Linux, you may need to enable buildx:
```bash
docker buildx create --use
```

### Permission denied errors with Docker

Add your user to the docker group:
```bash
sudo usermod -aG docker $USER
# Log out and back in for changes to take effect
```

Or use Podman which doesn't require root privileges.

## CI/CD Integration

The build script is designed to work in CI/CD environments:

```bash
# GitLab CI / GitHub Actions example
./build.sh --docker all

# Output artifacts are in dist/
ls -l dist/
```
