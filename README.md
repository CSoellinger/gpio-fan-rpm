# gpio-fan-rpm

A cli tool for measuring fan RPM using GPIO edge detection on Linux systems.

## Features

- Measure fan RPM via GPIO tachometer signal
- Support for multiple fans simultaneously (parallel measurements)
- Single measurement or continuous monitoring (watch mode)
- Multiple output formats: human-readable, numeric, JSON, collectd
- Uses libgpiod v2 for modern GPIO access
- Cross-platform builds with Docker/Podman support

## Quick Start

### On Linux

```bash
# Install dependencies
sudo apt-get install build-essential cmake libgpiod-dev

# Build
make

# Run
./build/gpio-fan-rpm --gpio=17
```

### On macOS (or Linux without dependencies)

```bash
# Install Podman or Docker
brew install podman  # or: brew install docker

# Build for ARM64 (Raspberry Pi, etc.)
make cross-arm64

# Build for x86_64
make cross-amd64

# Or build for all architectures
make all-arch

# Binaries will be in dist/
ls -lh dist/
```

## Usage

```bash
# Basic measurement
gpio-fan-rpm --gpio=17

# Specify pulses per revolution (default: 4)
gpio-fan-rpm --gpio=17 --pulses=2

# Continuous monitoring
gpio-fan-rpm --gpio=17 --watch

# Multiple fans at once
gpio-fan-rpm --gpio=17 --gpio=18 --gpio=27

# JSON output
gpio-fan-rpm --gpio=17 --json

# Numeric output (for scripting)
RPM=$(gpio-fan-rpm --gpio=17 --numeric)
echo "Fan speed: $RPM"
```

## Build System

This project uses CMake with Docker/Podman support for multi-architecture builds.

### Build Commands

```bash
# Native build (Linux only)
make                    # or: make release, make debug

# Container build (works on macOS/Linux)
make container

# Cross-compilation
make cross-arm64        # aarch64 (Cortex-A53)
make cross-amd64        # x86_64
make all-arch           # all architectures

# Advanced build script
./build.sh help         # See all options
./build.sh cross arm64  # Cross-compile
./build.sh all          # Build everything

# Custom version tag
./build.sh --tag v2.0.0 cross arm64
PKG_TAG=v1.5.0 make cross-arm64
```

See [BUILD.md](BUILD.md) for detailed build instructions.

## Requirements

### Runtime (Linux)
- Linux kernel with GPIO support
- libgpiod v2
- GPIO-connected fan with tachometer output

### Build
- **Linux**: CMake 3.10+, libgpiod-dev, build-essential
- **macOS**: Docker or Podman (native builds not supported)
- **Cross-compilation**: Docker/Podman with multi-arch support

## Hardware Setup

Connect your fan's tachometer wire to a GPIO pin on your device (Raspberry Pi, etc.):
- Fan tachometer → GPIO pin (e.g., GPIO 17)
- Fan GND → Ground
- Fan +12V → Power supply

## Environment Variables

- `GPIO_FAN_RPM_DURATION` - Measurement duration in seconds (default: 2)
- `GPIO_FAN_RPM_PULSES` - Pulses per revolution (default: 4)
- `DEBUG` - Enable debug output (set to "1" or "true")

## Documentation

- [BUILD.md](BUILD.md) - Detailed build instructions

## License

LGPL-3.0-or-later
