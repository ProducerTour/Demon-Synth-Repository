#!/bin/bash

# Demon Synth macOS DMG Builder
# Creates a professional DMG installer with GUI app

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
OUTPUT_DIR="$PROJECT_ROOT/Installer/Output"
DMG_NAME="DemonSynth_v1.0.0_macOS"
VOLUME_NAME="Demon Synth"

# Plugin paths
VST3_PATH="$BUILD_DIR/NulyBeatsPlugin_artefacts/Release/VST3/Demon Synth.vst3"
AU_PATH="$BUILD_DIR/NulyBeatsPlugin_artefacts/Release/AU/Demon Synth.component"
STANDALONE_PATH="$BUILD_DIR/NulyBeatsPlugin_artefacts/Release/Standalone/Demon Synth.app"

echo ""
echo "╔══════════════════════════════════════════════════════════════════╗"
echo "║           Demon Synth macOS DMG Builder                          ║"
echo "║                                                                  ║"
echo "║     © 2024 Nully Beats LLC / Producer Tour Publishing LLC        ║"
echo "╚══════════════════════════════════════════════════════════════════╝"
echo ""

# Check if build exists
if [ ! -d "$VST3_PATH" ]; then
    echo "Error: VST3 plugin not found. Please build first:"
    echo "  cd build && cmake --build . --config Release"
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

# First, create the installer app
echo "Step 1: Creating installer application..."
"$SCRIPT_DIR/create_installer_app.sh"

# Create temporary directory for DMG contents
TEMP_DIR=$(mktemp -d)
DMG_CONTENTS="$TEMP_DIR/DemonSynth"
mkdir -p "$DMG_CONTENTS"

echo ""
echo "Step 2: Copying files..."

# Copy Installer App
if [ -d "$SCRIPT_DIR/Install Demon Synth.app" ]; then
    cp -R "$SCRIPT_DIR/Install Demon Synth.app" "$DMG_CONTENTS/"
    echo "  ✓ Installer App"
fi

# Copy Uninstaller App
if [ -d "$SCRIPT_DIR/Uninstall Demon Synth.app" ]; then
    cp -R "$SCRIPT_DIR/Uninstall Demon Synth.app" "$DMG_CONTENTS/"
    echo "  ✓ Uninstaller App"
fi

# Copy VST3
if [ -d "$VST3_PATH" ]; then
    cp -R "$VST3_PATH" "$DMG_CONTENTS/"
    echo "  ✓ VST3"
fi

# Copy AU
if [ -d "$AU_PATH" ]; then
    cp -R "$AU_PATH" "$DMG_CONTENTS/"
    echo "  ✓ AU"
fi

# Copy Standalone
if [ -d "$STANDALONE_PATH" ]; then
    cp -R "$STANDALONE_PATH" "$DMG_CONTENTS/"
    echo "  ✓ Standalone"
fi

# Copy Resources (samples)
if [ -d "$PROJECT_ROOT/Resources/Samples" ]; then
    mkdir -p "$DMG_CONTENTS/Samples"
    cp -R "$PROJECT_ROOT/Resources/Samples/"* "$DMG_CONTENTS/Samples/"
    echo "  ✓ Samples"
fi

# Copy License
if [ -f "$PROJECT_ROOT/Installer/LICENSE.txt" ]; then
    cp "$PROJECT_ROOT/Installer/LICENSE.txt" "$DMG_CONTENTS/"
    echo "  ✓ License"
fi

# Create README
cat > "$DMG_CONTENTS/README.txt" << 'README'
╔══════════════════════════════════════════════════════════════════╗
║                      DEMON SYNTH                                  ║
║                     by Nully Beats                                ║
╚══════════════════════════════════════════════════════════════════╝

INSTALLATION
────────────
Double-click "Install Demon Synth" to run the installer.

The installer will:
  1. Ask you to accept the License Agreement
  2. Let you choose installation type
  3. Let you choose where to store sample presets
  4. Install everything automatically

MANUAL INSTALLATION
───────────────────
If you prefer to install manually:

VST3: Copy "Demon Synth.vst3" to:
      ~/Library/Audio/Plug-Ins/VST3/

AU:   Copy "Demon Synth.component" to:
      ~/Library/Audio/Plug-Ins/Components/

App:  Copy "Demon Synth.app" to:
      /Applications/

Samples: Copy the "Samples" folder to:
         ~/Library/Application Support/NullyBeats/Demon Synth/Samples/

GATEKEEPER BYPASS
─────────────────
If macOS blocks the plugin, run in Terminal:
  xattr -rd com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Demon\ Synth.vst3
  xattr -rd com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/Demon\ Synth.component

Or go to: System Preferences → Security & Privacy → Allow

SUPPORT
───────
Website: https://nullybeats.com
Email: support@nullybeats.com

© 2024 Nolan Griffis p/k/a Nully Beats
Nully Beats LLC / Producer Tour Publishing LLC
All Rights Reserved.
README

echo ""
echo "Step 3: Creating DMG..."

# Remove old DMG if exists
rm -f "$OUTPUT_DIR/$DMG_NAME.dmg"

# Create DMG with custom settings
hdiutil create -volname "$VOLUME_NAME" \
    -srcfolder "$DMG_CONTENTS" \
    -ov -format UDZO \
    -imagekey zlib-level=9 \
    "$OUTPUT_DIR/$DMG_NAME.dmg"

# Cleanup
rm -rf "$TEMP_DIR"

# Get file size
DMG_SIZE=$(ls -lh "$OUTPUT_DIR/$DMG_NAME.dmg" | awk '{print $5}')

echo ""
echo "╔══════════════════════════════════════════════════════════════════╗"
echo "║  DMG created successfully!                                       ║"
echo "╚══════════════════════════════════════════════════════════════════╝"
echo ""
echo "Output: $OUTPUT_DIR/$DMG_NAME.dmg"
echo "Size:   $DMG_SIZE"
echo ""
