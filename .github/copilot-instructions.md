# RPMegaFighter - RP6502 Project

## Project Overview
This is an RP6502 picocomputer project built with llvm-mos-sdk and CMake.

## Build Instructions
1. Ensure llvm-mos toolchain is installed at `~/Software/rp6502/llvm-mos/bin`
2. Run CMake configuration: `cmake -B build -G Ninja`
3. Build the project: `cmake --build build`

## Development Guidelines
- C code should be placed in `src/` directory
- Binary assets should be placed in `images/` directory
- Use RP6502 API for hardware interaction
- Follow the existing code style in the template

## Checklist
- [x] Create copilot-instructions.md file
- [x] Copy template project structure
- [x] Update project name in CMakeLists.txt
- [x] Update source files
- [x] Install required VS Code extensions
- [x] Verify project builds
