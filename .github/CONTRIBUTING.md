# Contributing to gpio-fan-rpm-cli

Thank you for your interest in contributing to gpio-fan-rpm-cli! This document provides guidelines and information for contributors.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [Project Architecture](#project-architecture)
- [Coding Standards](#coding-standards)
- [Making Changes](#making-changes)
- [Testing](#testing)
- [Submitting Changes](#submitting-changes)

## Code of Conduct

We expect all contributors to be respectful and constructive. Please:

- Be welcoming to newcomers
- Be respectful of differing viewpoints and experiences
- Accept constructive criticism gracefully
- Focus on what is best for the community

## Getting Started

### Prerequisites

- **For Linux development:**
  - CMake 3.10+
  - GCC or Clang
  - libgpiod v2 development libraries (`libgpiod-dev`)
  - pthread support

- **For macOS or containerized development:**
  - Docker or Podman
  - (Native builds not supported on macOS due to Linux-specific GPIO requirements)

### Building the Project

See [BUILD.md](../BUILD.md) for detailed build instructions. Quick start:

```bash
# Native build (Linux)
make

# Container build (Linux/macOS)
make container

# Cross-compilation
make cross-arm64  # or cross-amd64, all-arch
```

## Development Setup

1. **Fork and clone the repository:**
   ```bash
   git clone https://github.com/yourusername/gpio-fan-rpm-cli.git
   cd gpio-fan-rpm-cli
   ```

2. **Build in debug mode:**
   ```bash
   make debug
   # or
   ./build.sh --debug local
   ```

3. **Run with debug output:**
   ```bash
   DEBUG=1 ./build/gpio-fan-rpm --gpio=17
   # or
   ./build/gpio-fan-rpm --debug --gpio=17
   ```

## Project Architecture

### Code Organization

The codebase uses a **layered architecture** to abstract GPIO operations:

1. **libgpiod Layer** (`chip.c`, `line.c`)
   - Direct wrappers around libgpiod v2 API
   - Chip discovery and line request management

2. **GPIO Context Layer** (`gpio.c`)
   - Higher-level GPIO operations
   - Core measurement algorithm
   - Thread management for parallel measurements

3. **Application Layer** (`measure.c`, `watch.c`)
   - Single measurement mode
   - Continuous monitoring mode

### Key Components

- **src/chip.c** - GPIO chip discovery and management
- **src/line.c** - GPIO line operations and edge detection
- **src/gpio.c** - High-level GPIO context and measurement logic
- **src/measure.c** - Single measurement orchestration
- **src/watch.c** - Continuous monitoring mode
- **src/args.c** - Command-line argument parsing
- **src/format.c** - Output formatting (default, JSON, numeric, collectd)
- **src/utils.c** - Utility functions

### Threading Model

- Each GPIO gets its own pthread for parallel measurements
- Shared state uses `pthread_mutex_t` and `pthread_cond_t`
- Global `print_mutex` serializes output across threads
- Global volatile `stop` flag enables graceful shutdown

## Coding Standards

### General Guidelines

- **Language:** C (C11 standard)
- **Style:** Consistent with existing codebase
- **Indentation:** Tabs for indentation, spaces for alignment
- **Line length:** No strict limit, but keep it reasonable (80-120 chars preferred)
- **Comments:** Use clear, concise comments for non-obvious logic

### Return Value Conventions

Follow these strict conventions (documented in `chip.h`):

- **int functions:** Return `0` on success, `-1` on error
  - Exception: `parse_arguments()` returns `1` for help/version
  - Exception: wait functions return `0=timeout`, `1=event ready`, `-1=error`
- **Pointer functions:** Return valid pointer on success, `NULL` on error
- **double functions:** Return measured value, `-1.0` on interrupt, `0.0` if no data

### Memory Management

- Caller must free strings returned by `format_*` functions
- Thread args are allocated by parent, freed by thread function
- Always check for `NULL` before dereferencing pointers
- Free all allocated memory before returning from functions

### Error Handling

- Debug output goes to `stderr` via `fprintf`
- Errors without debug show hint: "use --debug for details"
- Use descriptive error messages
- Always clean up resources on error paths

### Security Considerations

When contributing, be mindful of:

- **Command injection:** Validate all external input
- **Buffer overflows:** Use safe string functions, check bounds
- **Race conditions:** Proper mutex usage in threaded code
- **Resource leaks:** Clean up file descriptors, memory, and GPIO resources

## Making Changes

### Before You Start

1. Check existing issues and pull requests
2. For major changes, open an issue first to discuss your approach
3. Create a feature branch from `main`:
   ```bash
   git checkout -b feature/your-feature-name
   ```

### Development Workflow

1. **Make your changes** in small, logical commits
2. **Build and test** your changes:
   ```bash
   make debug
   ./build/gpio-fan-rpm --gpio=17 --debug
   ```
3. **Test across architectures** if possible:
   ```bash
   make all-arch
   ```
4. **Update documentation** if needed (README.md, BUILD.md, CLAUDE.md)

### Commit Messages

Write clear, descriptive commit messages:

```
Short summary (50 chars or less)

More detailed explanation if needed. Wrap at 72 characters.
Explain what changed and why, not how (the code shows how).

- Bullet points are fine
- Use present tense: "Add feature" not "Added feature"
```

## Testing

### Manual Testing

Since this is a hardware-interfacing utility, testing requires GPIO hardware:

1. **Test single measurement:**
   ```bash
   ./build/gpio-fan-rpm --gpio=17
   ```

2. **Test multiple GPIOs:**
   ```bash
   ./build/gpio-fan-rpm --gpio=17 --gpio=18 --gpio=27
   ```

3. **Test watch mode:**
   ```bash
   ./build/gpio-fan-rpm --gpio=17 --watch
   ```

4. **Test output formats:**
   ```bash
   ./build/gpio-fan-rpm --gpio=17 --json
   ./build/gpio-fan-rpm --gpio=17 --numeric
   ./build/gpio-fan-rpm --gpio=17 --collectd
   ```

5. **Test error conditions:**
   - Invalid GPIO numbers
   - Missing GPIO device
   - Permission denied scenarios
   - Signal interruption (Ctrl+C)

### Build Testing

Ensure builds work across configurations:

```bash
# Debug build
make debug

# Release build
make release

# Container build
make container

# Cross-platform builds
make cross-arm64
make cross-amd64
```

### Environment Variable Testing

Test environment variable overrides:

```bash
GPIO_FAN_RPM_DURATION=5 ./build/gpio-fan-rpm --gpio=17
GPIO_FAN_RPM_PULSES=2 ./build/gpio-fan-rpm --gpio=17
DEBUG=1 ./build/gpio-fan-rpm --gpio=17
```

## Submitting Changes

### Pull Request Process

1. **Ensure your code builds** without warnings:
   ```bash
   make clean
   make
   ```

2. **Update documentation** as needed

3. **Push your branch** to your fork:
   ```bash
   git push origin feature/your-feature-name
   ```

4. **Open a Pull Request** with:
   - Clear description of changes
   - Motivation for the change
   - Any relevant issue numbers
   - Testing performed
   - Screenshots/output examples if applicable

### Pull Request Checklist

- [ ] Code builds successfully
- [ ] No compiler warnings introduced
- [ ] Code follows project conventions
- [ ] Documentation updated if needed
- [ ] Testing performed (describe in PR)
- [ ] Commit messages are clear and descriptive
- [ ] Changes are focused and logical

### Review Process

- Maintainers will review your PR
- Address any feedback or requested changes
- Once approved, your PR will be merged

## Questions?

If you have questions:

1. Check existing issues and documentation
2. Open a new issue for discussion
3. Be clear and specific about your question

## License

By contributing to gpio-fan-rpm-cli, you agree that your contributions will be licensed under the project's LGPL-3.0-or-later license.

---

Thank you for contributing to gpio-fan-rpm-cli!
