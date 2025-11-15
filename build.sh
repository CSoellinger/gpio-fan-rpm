#!/usr/bin/env bash
# Build script for gpio-fan-rpm
# Supports native builds and containerized multi-architecture builds

set -e

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Default values
BUILD_TYPE="${BUILD_TYPE:-Release}"
CONTAINER_ENGINE="${CONTAINER_ENGINE:-}"
OUTPUT_DIR="${OUTPUT_DIR:-dist}"
PKG_TAG="${PKG_TAG:-}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Helper functions
info() {
    echo -e "${GREEN}[INFO]${NC} $*"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $*"
}

error() {
    echo -e "${RED}[ERROR]${NC} $*"
    exit 1
}

# Detect container engine
detect_container_engine() {
    if [ -n "$CONTAINER_ENGINE" ]; then
        return
    fi

    if command -v podman &> /dev/null; then
        CONTAINER_ENGINE="podman"
    elif command -v docker &> /dev/null; then
        CONTAINER_ENGINE="docker"
    else
        warn "No container engine found (podman/docker)"
        CONTAINER_ENGINE=""
    fi
}

# Print usage
usage() {
    cat << EOF
Usage: $0 [OPTIONS] [COMMAND]

Commands:
    local           Build natively on this system
    container       Build in container for current architecture
    cross <arch>    Cross-build for specified architecture
    all             Build for all supported architectures
    clean           Clean build artifacts
    help            Show this help message

Options:
    --release       Build in release mode (default)
    --debug         Build in debug mode
    --podman        Use podman instead of docker
    --docker        Use docker instead of podman
    --output DIR    Output directory (default: dist)
    --tag TAG       Set package version tag (e.g., v1.2.3, default: 1.0.0)

Supported architectures:
    amd64, x86_64   - x86_64 / amd64
    arm64, aarch64  - aarch64 (Cortex-A53)

Examples:
    $0 local                    # Build natively
    $0 container                # Build in container
    $0 cross arm64              # Cross-build for arm64
    $0 all                      # Build for all architectures
    $0 --debug local            # Debug build
    $0 --tag v2.0.0 cross arm64 # Build with custom version

Environment variables:
    BUILD_TYPE          - Release or Debug (default: Release)
    CONTAINER_ENGINE    - podman or docker
    OUTPUT_DIR          - Output directory (default: dist)
    PKG_TAG             - Package version tag (e.g., v1.2.3)

EOF
}

# Build locally (native)
build_local() {
    info "Building locally for native architecture..."

    # Create build directory
    mkdir -p build
    cd build

    # Configure
    info "Configuring with CMake..."
    CMAKE_OPTS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
    if [ -n "$PKG_TAG" ]; then
        CMAKE_OPTS="$CMAKE_OPTS -DPKG_TAG=$PKG_TAG"
        info "Using package tag: $PKG_TAG"
    fi
    cmake $CMAKE_OPTS ..

    # Build
    info "Building..."
    make -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

    # Copy to output directory
    cd ..
    mkdir -p "$OUTPUT_DIR"
    cp build/gpio-fan-rpm "$OUTPUT_DIR/gpio-fan-rpm-$(uname -m)"

    info "Build complete: $OUTPUT_DIR/gpio-fan-rpm-$(uname -m)"
}

# Build in container (native to container)
build_container() {
    detect_container_engine

    if [ -z "$CONTAINER_ENGINE" ]; then
        error "No container engine available. Install podman or docker."
    fi

    info "Building in container using $CONTAINER_ENGINE..."

    # Build container image
    $CONTAINER_ENGINE build -t gpio-fan-rpm-builder .

    # Extract binary from container
    mkdir -p "$OUTPUT_DIR"
    CONTAINER_ID=$($CONTAINER_ENGINE create gpio-fan-rpm-builder)
    $CONTAINER_ENGINE cp "$CONTAINER_ID:/usr/local/bin/gpio-fan-rpm" "$OUTPUT_DIR/gpio-fan-rpm-$(uname -m)"
    $CONTAINER_ENGINE rm "$CONTAINER_ID"

    info "Build complete: $OUTPUT_DIR/gpio-fan-rpm-$(uname -m)"
}

# Cross-build for specific architecture
build_cross() {
    local ARCH="$1"

    if [ -z "$ARCH" ]; then
        error "Architecture not specified. Use: amd64, arm64, aarch64"
    fi

    # Normalize architecture names
    case "$ARCH" in
        amd64|x86_64)
            PLATFORM="linux/amd64"
            TARGET_ARCH_NORM="amd64"
            OUTPUT_NAME="gpio-fan-rpm-x86_64"
            ;;
        arm64|aarch64)
            PLATFORM="linux/arm64"
            TARGET_ARCH_NORM="arm64"
            OUTPUT_NAME="gpio-fan-rpm-aarch64"
            ;;
        *)
            error "Unsupported architecture: $ARCH. Use: amd64, arm64, aarch64"
            ;;
    esac

    detect_container_engine

    if [ -z "$CONTAINER_ENGINE" ]; then
        error "No container engine available. Install podman or docker."
    fi

    info "Cross-building for $ARCH using $CONTAINER_ENGINE..."
    if [ -n "$PKG_TAG" ]; then
        info "Using package tag: $PKG_TAG"
    fi

    # Check if buildx/buildah is available for docker/podman
    if [ "$CONTAINER_ENGINE" = "docker" ]; then
        if ! docker buildx version &> /dev/null; then
            warn "docker buildx not available, trying regular build with --platform"
        fi
        BUILD_CMD="docker buildx build --platform=$PLATFORM"
    else
        BUILD_CMD="podman build --platform=$PLATFORM"
    fi

    # Prepare build arguments
    BUILD_ARGS="--build-arg TARGET_ARCH=$TARGET_ARCH_NORM"
    if [ -n "$PKG_TAG" ]; then
        BUILD_ARGS="$BUILD_ARGS --build-arg PKG_TAG=$PKG_TAG"
    fi

    # Build for target platform
    $BUILD_CMD -f Dockerfile.cross $BUILD_ARGS -t "gpio-fan-rpm-builder-$ARCH" .

    # Extract binary
    mkdir -p "$OUTPUT_DIR"
    CONTAINER_ID=$($CONTAINER_ENGINE create "gpio-fan-rpm-builder-$ARCH")
    $CONTAINER_ENGINE cp "$CONTAINER_ID:/usr/local/bin/gpio-fan-rpm" "$OUTPUT_DIR/$OUTPUT_NAME"
    $CONTAINER_ENGINE rm "$CONTAINER_ID"

    info "Build complete: $OUTPUT_DIR/$OUTPUT_NAME"
}

# Build for all architectures
build_all() {
    info "Building for all supported architectures..."

    build_cross amd64
    build_cross arm64

    info "All builds complete. Binaries in $OUTPUT_DIR/"
    ls -lh "$OUTPUT_DIR/"
}

# Clean build artifacts
clean() {
    info "Cleaning build artifacts..."
    rm -rf build "$OUTPUT_DIR"
    info "Clean complete"
}

# Parse arguments
COMMAND=""
while [ $# -gt 0 ]; do
    case "$1" in
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --podman)
            CONTAINER_ENGINE="podman"
            shift
            ;;
        --docker)
            CONTAINER_ENGINE="docker"
            shift
            ;;
        --output)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --tag)
            PKG_TAG="$2"
            shift 2
            ;;
        local|container|cross|all|clean|help)
            COMMAND="$1"
            shift
            break
            ;;
        *)
            error "Unknown option: $1. Use '$0 help' for usage."
            ;;
    esac
done

# Execute command
case "$COMMAND" in
    local)
        build_local
        ;;
    container)
        build_container
        ;;
    cross)
        build_cross "$@"
        ;;
    all)
        build_all
        ;;
    clean)
        clean
        ;;
    help|"")
        usage
        ;;
    *)
        error "Unknown command: $COMMAND"
        ;;
esac
