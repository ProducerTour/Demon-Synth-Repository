#!/bin/bash

# Create Demon Synth Installer and Uninstaller Apps from AppleScript
# This creates proper macOS app bundles

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Create Installer App
INSTALLER_NAME="Install Demon Synth.app"
INSTALLER_PATH="$SCRIPT_DIR/$INSTALLER_NAME"

echo "Creating installer app..."
rm -rf "$INSTALLER_PATH"
osacompile -o "$INSTALLER_PATH" "$SCRIPT_DIR/DemonSynthInstaller.applescript"

if [ $? -eq 0 ]; then
    chmod +x "$INSTALLER_PATH/Contents/MacOS/"*
    echo "✓ Installer app created: $INSTALLER_PATH"
else
    echo "✗ Failed to create installer app"
    exit 1
fi

# Create Uninstaller App
UNINSTALLER_NAME="Uninstall Demon Synth.app"
UNINSTALLER_PATH="$SCRIPT_DIR/$UNINSTALLER_NAME"

echo "Creating uninstaller app..."
rm -rf "$UNINSTALLER_PATH"
osacompile -o "$UNINSTALLER_PATH" "$SCRIPT_DIR/DemonSynthUninstaller.applescript"

if [ $? -eq 0 ]; then
    chmod +x "$UNINSTALLER_PATH/Contents/MacOS/"*
    echo "✓ Uninstaller app created: $UNINSTALLER_PATH"
else
    echo "✗ Failed to create uninstaller app"
    exit 1
fi

echo "✓ Done!"
