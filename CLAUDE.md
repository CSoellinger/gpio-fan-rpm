# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

gpio-fan-rpm-cli is a C utility for measuring fan RPM using GPIO edge detection on Linux systems. It uses libgpiod v2 for GPIO operations and supports single measurements or continuous monitoring (watch mode) of multiple fan tachometer inputs simultaneously.

## Build System

This project uses CMake with Docker/Podman support for multi-architecture builds.

### Quick Build Commands

```bash
# Native build
make                    # or: make release, make debug
./build.sh local

# Container build (recommended for cross-platform)
make container
./build.sh container

# Cross-compilation
make cross-arm64        # aarch64 (Cortex-A53)
make cross-amd64        # x86_64
make all-arch           # all architectures

# Using build script directly
./build.sh cross arm64
./build.sh all
```

### Build System Components

- **CMakeLists.txt** - Main CMake configuration
- **Makefile** - Convenience wrapper for common tasks
- **build.sh** - Advanced multi-arch build script
- **Dockerfile** - Native architecture container build
- **Dockerfile.cross** - Cross-compilation container build
- **cmake/toolchains/** - CMake toolchain files for cross-compilation

### Build Requirements

- **Native builds** (Linux only): CMake 3.10+, libgpiod-dev, build-essential
- **Container builds**: Docker or Podman (auto-detected)
- **Cross-builds**: Docker buildx or Podman with qemu-user-static
- **macOS**: Native builds not supported - use container builds instead

### Build Output

Binaries are placed in:
- **Native builds**: `build/gpio-fan-rpm`
- **Container/cross builds**: `dist/gpio-fan-rpm-{arch}`

### Version Tagging

The binary version can be customized via CMake parameter:
```bash
cmake -DPKG_TAG=v2.0.0 ..
./build.sh --tag v2.0.0 cross arm64
PKG_TAG=v2.0.0 make cross-arm64
```

See BUILD.md for detailed build instructions and troubleshooting.

## Architecture

### Layered Abstraction

The codebase uses a layered architecture to abstract GPIO operations:

1. **libgpiod Layer** (chip.c, line.c): Direct wrappers around libgpiod v2 API
   - `chip.c` - chip discovery, opening, and metadata retrieval
   - `line.c` - line request management and edge event operations

2. **GPIO Context Layer** (gpio.c): Higher-level GPIO operations
   - `gpio_context_t` - encapsulates chip + line request + event fd
   - `gpio_init()` / `gpio_cleanup()` - lifecycle management
   - `gpio_measure_rpm()` - core measurement algorithm
   - `gpio_thread_fn()` - pthread wrapper for parallel measurements

3. **Application Layer** (measure.c, watch.c): Orchestration modes
   - `measure.c` - single measurement with parallel thread coordination
   - `watch.c` - continuous monitoring with thread management

### Threading Model

- Each GPIO gets its own pthread for parallel measurement
- Shared state uses `pthread_mutex_t` and `pthread_cond_t` for synchronization
- Results array + finished flags track completion status
- Global `print_mutex` serializes output across threads
- Global volatile `stop` flag enables graceful SIGINT/SIGTERM shutdown

### Auto-detection Strategy

The chip auto-detection (chip.c:chip_auto_detect) searches common GPIO chip paths:
- `/dev/gpiochip0`, `/dev/gpiochip1`, ... `/dev/gpiochip4`
- Returns first chip where the requested GPIO line is available
- Result is cached and reused across multiple GPIOs in same invocation

### Output Formatting

`format.c` provides four output modes (output_mode_t enum):
- `MODE_DEFAULT` - human-readable "GPIO X: Y RPM"
- `MODE_NUMERIC` - raw number only (for shell capture)
- `MODE_JSON` - JSON object/array
- `MODE_COLLECTD` - collectd PUTVAL format

## Important Conventions

### Return Value Patterns

This codebase follows strict return conventions documented in chip.h:

- **int functions**: 0 on success, -1 on error
  - Exception: `parse_arguments()` returns 1 for help/version
  - Exception: wait functions return 0=timeout, 1=event ready, -1=error
- **Pointer functions**: valid pointer on success, NULL on error
- **double functions**: measured value, -1.0 on interrupt, 0.0 if no data

### Memory Management

- Caller must free strings returned by format_* functions
- Thread args are allocated by parent, freed by thread function (gpio_thread_fn)
- Auto-detected chipname is allocated and must be freed in main()

### Error Handling

- Debug output goes to stderr via fprintf
- Debug mode is controlled by `--debug` flag or `DEBUG=1` env var
- Errors without debug show hint: "use --debug for details"

## Environment Variables

Defaults can be overridden via environment:
- `GPIO_FAN_RPM_DURATION` - measurement duration (seconds)
- `GPIO_FAN_RPM_PULSES` - pulses per revolution
- `DEBUG` - enable debug output (set to "1" or "true")

## Signal Handling

- SIGINT/SIGTERM handlers set global `stop` flag
- Measurement loops check `stop` flag and exit gracefully
- Threads detect interruption and return -1.0 from gpio_measure_rpm()

## Key Constraints

- Minimum measurement duration: 2 seconds (1s warmup + 1s measurement)
- Maximum measurement duration: 3600 seconds
- GPIO range: 0-999
- Pulses per revolution: 1-100
- Maximum GPIOs per invocation: 10 (soft limit in args.c)
