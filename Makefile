# Makefile for gpio-fan-rpm
# Simple wrapper around CMake for common operations

# Detect platform
UNAME_S := $(shell uname -s)

# Default target
.PHONY: all
ifeq ($(UNAME_S),Darwin)
all: macos-help
else
all: release
endif

# Build in release mode
.PHONY: release
release:
	@mkdir -p build
	@cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && $(MAKE)
	@echo "Binary: build/gpio-fan-rpm"

# Build in debug mode
.PHONY: debug
debug:
	@mkdir -p build
	@cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && $(MAKE)
	@echo "Binary: build/gpio-fan-rpm"

# Clean build artifacts
.PHONY: clean
clean:
	@rm -rf build dist
	@echo "Cleaned build artifacts"

# Install to system
.PHONY: install
install: release
	@cd build && $(MAKE) install
	@echo "Installed to system"

# Build in container (native architecture)
.PHONY: container
container:
	@./build.sh container

# Cross-build for aarch64
.PHONY: cross-arm64
cross-arm64:
	@./build.sh cross arm64

# Cross-build for x86_64
.PHONY: cross-amd64
cross-amd64:
	@./build.sh cross amd64

# Build for all architectures
.PHONY: all-arch
all-arch:
	@./build.sh all

# macOS-specific help
.PHONY: macos-help
macos-help:
	@echo ""
	@echo "╔════════════════════════════════════════════════════════════════╗"
	@echo "║  macOS Detected - Native Build Not Supported                  ║"
	@echo "║                                                                ║"
	@echo "║  This project requires libgpiod which is Linux-specific.      ║"
	@echo "║  Use container builds instead:                                ║"
	@echo "║                                                                ║"
	@echo "║    make container      # Build in container                   ║"
	@echo "║    make cross-arm64    # Cross-compile for ARM64              ║"
	@echo "║    make cross-amd64    # Cross-compile for x86_64             ║"
	@echo "║    make all-arch       # Build all architectures              ║"
	@echo "║                                                                ║"
	@echo "║  Or use the build script:                                     ║"
	@echo "║    ./build.sh help                                            ║"
	@echo "╚════════════════════════════════════════════════════════════════╝"
	@echo ""

# Help
.PHONY: help
help:
ifeq ($(UNAME_S),Darwin)
	@$(MAKE) macos-help
	@echo "Additional targets:"
else
	@echo "Available targets:"
	@echo "  make               - Build in release mode (native)"
	@echo "  make release       - Build in release mode (native)"
	@echo "  make debug         - Build in debug mode (native)"
endif
	@echo "  make clean         - Clean build artifacts"
	@echo "  make install       - Install to system"
	@echo "  make container     - Build in container"
	@echo "  make cross-arm64   - Cross-build for aarch64"
	@echo "  make cross-amd64   - Cross-build for x86_64"
	@echo "  make all-arch      - Build for all architectures"
	@echo "  make help          - Show this help"
	@echo ""
	@echo "For more options, use: ./build.sh help"
