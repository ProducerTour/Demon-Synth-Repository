#!/bin/bash

# NulyBeats Plugin Setup Script
# This script clones JUCE and sets up the build environment

set -e

echo "=========================================="
echo "  NulyBeats Synth Plugin Setup"
echo "=========================================="

# Check for required tools
command -v cmake >/dev/null 2>&1 || { echo "CMake is required but not installed. Install with: brew install cmake"; exit 1; }
command -v git >/dev/null 2>&1 || { echo "Git is required but not installed."; exit 1; }

# Clone JUCE if not present
if [ ! -d "JUCE" ]; then
    echo "Cloning JUCE framework..."
    git clone --depth 1 --branch 7.0.9 https://github.com/juce-framework/JUCE.git
else
    echo "JUCE already present."
fi

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo ""
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

echo ""
echo "=========================================="
echo "  Setup Complete!"
echo "=========================================="
echo ""
echo "To build the plugin, run:"
echo "  cd build && cmake --build . --config Release"
echo ""
echo "The VST3 plugin will be in:"
echo "  build/NulyBeatsPlugin_artefacts/Release/VST3/"
echo ""
echo "For development (Debug build):"
echo "  cmake .. -DCMAKE_BUILD_TYPE=Debug"
echo "  cmake --build . --config Debug"
echo ""
