# Cash Register

Cash Register is a Qt6-based desktop application for managing basic cash register
operations, sales and transactions.

## Features

- Simple cash register management
- Sales and transaction tracking
- Group-based organization
- Print support
- Multi-language support (English, Italian)
- Qt6 Widgets interface

## Build Dependencies

- Qt6 (qt6-base)
- CMake
- Ninja

## Build Instructions

```bash
mkdir build
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
