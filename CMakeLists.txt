cmake_minimum_required(VERSION 3.10)
project(gpio-fan-rpm C)

# Version information
set(PROJECT_VERSION "dev" CACHE STRING "Project version")

# Package tag (version string shown in binary)
# Can be overridden with: cmake -DPKG_TAG=v1.2.3 ..
if(NOT DEFINED PKG_TAG)
    set(PKG_TAG "${PROJECT_VERSION}")
endif()
message(STATUS "Package tag: ${PKG_TAG}")

# Build timestamp (shown in --version output)
# Can be overridden with: cmake -DBUILD_TIMESTAMP="2025-11-16 12:34:56" ..
if(NOT DEFINED BUILD_TIMESTAMP)
    string(TIMESTAMP BUILD_TIMESTAMP "%b %d %Y %H:%M:%S" UTC)
endif()
message(STATUS "Build timestamp: ${BUILD_TIMESTAMP}")

# C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")

# Release flags optimized for size
set(CMAKE_C_FLAGS_RELEASE "-Os -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,--gc-sections -Wl,-s")

# Enable Link-Time Optimization for release builds
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

# Option for static linking (useful for OpenWrt/embedded systems)
option(BUILD_STATIC "Build static binary" OFF)
if(BUILD_STATIC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(BUILD_SHARED_LIBS OFF)
endif()

# Source files
set(SOURCES
    src/main.c
    src/args.c
    src/gpio.c
    src/chip.c
    src/line.c
    src/format.c
    src/stats.c
    src/measure.c
    src/watch.c
)

# Include directory
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

# Executable
add_executable(gpio-fan-rpm ${SOURCES})

# Add version and build timestamp as compile definitions
target_compile_definitions(gpio-fan-rpm PRIVATE
    PKG_TAG=${PKG_TAG}
    BUILD_TIMESTAMP="${BUILD_TIMESTAMP}"
)

# Find required libraries
find_package(Threads REQUIRED)

# Find libgpiod (Linux-specific)
find_library(GPIOD_LIBRARY gpiod)

# Note: json-c dependency removed - simple JSON now uses snprintf directly

if(NOT GPIOD_LIBRARY)
    if(APPLE)
        message(WARNING "
╔════════════════════════════════════════════════════════════════╗
║  libgpiod not found (expected on macOS)                        ║
║                                                                ║
║  This project requires libgpiod which is Linux-specific.      ║
║  On macOS, use container builds instead:                      ║
║                                                                ║
║    ./build.sh container      # Build in container             ║
║    ./build.sh cross arm64    # Cross-compile for ARM64        ║
║    ./build.sh all            # Build all architectures        ║
║                                                                ║
║  Or use Docker/Podman directly:                               ║
║    docker build -t gpio-fan-rpm .                             ║
║    podman build -t gpio-fan-rpm .                             ║
╚════════════════════════════════════════════════════════════════╝
")
        message(FATAL_ERROR "Cannot build natively on macOS. Use container builds.")
    else()
        message(FATAL_ERROR "
libgpiod not found!

Please install libgpiod development package:
  Debian/Ubuntu: sudo apt-get install libgpiod-dev
  Fedora/RHEL:   sudo dnf install libgpiod-devel
  Arch Linux:    sudo pacman -S libgpiod

Or use container builds:
  ./build.sh container
")
    endif()
endif()

# Link libraries
target_link_libraries(gpio-fan-rpm
    ${GPIOD_LIBRARY}
    Threads::Threads
    m
)

# Installation
install(TARGETS gpio-fan-rpm DESTINATION bin)

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Flags: ${CMAKE_C_FLAGS}")
message(STATUS "Target architecture: ${CMAKE_SYSTEM_PROCESSOR}")
