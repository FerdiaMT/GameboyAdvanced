#!/bin/bash

# Build script for GameboyAdvanced project

set -e

cmake --preset debug
cmake --build --preset debug

echo "Build complete! Executable created at: ./GBA"
