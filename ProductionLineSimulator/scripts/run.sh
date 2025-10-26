#!/bin/bash

# Build and run script for Production Line Simulator
# Usage: ./scripts/run.sh [build_type]

set -e  # Exit on any error

BUILD_TYPE=${1:-Release}
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

echo "=== Production Line Simulator Build Script ==="
echo "Project root: $PROJECT_ROOT"
echo "Build type: $BUILD_TYPE"
echo "Build directory: $BUILD_DIR"

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Check for required tools
echo "Checking build requirements..."

if ! command -v cmake &> /dev/null; then
    echo "Error: CMake is not installed or not in PATH"
    echo "Install with: sudo apt install cmake"
    exit 1
fi

if ! command -v qmake6 &> /dev/null && ! command -v qmake-qt6 &> /dev/null; then
    echo "Warning: Qt 6 qmake not found in PATH"
    echo "Make sure Qt 6 development packages are installed:"
    echo "  sudo apt install qt6-base-dev qt6-charts-dev"
fi

# Configure project
echo "Configuring CMake..."
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      "$PROJECT_ROOT"

# Build project
echo "Building project..."
CPU_COUNT=$(nproc)
cmake --build . --parallel "$CPU_COUNT"

if [ $? -eq 0 ]; then
    echo "Build successful!"
    
    # Check if executable exists
    if [ -f "./ProductionLineSimulator" ]; then
        echo "=== Running Production Line Simulator ==="
        ./ProductionLineSimulator
    else
        echo "Error: Executable not found!"
        exit 1
    fi
else
    echo "Build failed!"
    exit 1
fi
