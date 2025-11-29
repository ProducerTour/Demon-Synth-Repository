#!/bin/bash

# Demon Synth macOS Installer
# Interactive installer with Terms of Service and directory selection

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALLER_ROOT="$SCRIPT_DIR"

# Default paths
DEFAULT_VST3_PATH="$HOME/Library/Audio/Plug-Ins/VST3"
DEFAULT_AU_PATH="$HOME/Library/Audio/Plug-Ins/Components"
DEFAULT_APP_PATH="/Applications"
DEFAULT_SAMPLES_PATH="$HOME/Library/Application Support/NullyBeats/Demon Synth/Samples"

# Config file location (plugin reads this)
CONFIG_DIR="$HOME/Library/Application Support/NullyBeats/Demon Synth"
CONFIG_FILE="$CONFIG_DIR/config.json"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color
BOLD='\033[1m'

clear
echo -e "${RED}"
echo "╔══════════════════════════════════════════════════════════════════╗"
echo "║                                                                  ║"
echo "║     ██████╗ ███████╗███╗   ███╗ ██████╗ ███╗   ██╗              ║"
echo "║     ██╔══██╗██╔════╝████╗ ████║██╔═══██╗████╗  ██║              ║"
echo "║     ██║  ██║█████╗  ██╔████╔██║██║   ██║██╔██╗ ██║              ║"
echo "║     ██║  ██║██╔══╝  ██║╚██╔╝██║██║   ██║██║╚██╗██║              ║"
echo "║     ██████╔╝███████╗██║ ╚═╝ ██║╚██████╔╝██║ ╚████║              ║"
echo "║     ╚═════╝ ╚══════╝╚═╝     ╚═╝ ╚═════╝ ╚═╝  ╚═══╝              ║"
echo "║                      SYNTH                                       ║"
echo "║                                                                  ║"
echo "║                    by Nully Beats                                ║"
echo "║                                                                  ║"
echo "╚══════════════════════════════════════════════════════════════════╝"
echo -e "${NC}"
echo ""
echo -e "${CYAN}Welcome to the Demon Synth Installer${NC}"
echo -e "Version 1.0.0"
echo ""
echo "════════════════════════════════════════════════════════════════════"
echo ""

# ============================================================================
# STEP 1: Terms of Service
# ============================================================================
echo -e "${YELLOW}${BOLD}STEP 1: Terms of Service${NC}"
echo ""
echo "Please read and accept the following license agreement:"
echo ""
echo "────────────────────────────────────────────────────────────────────"

if [ -f "$INSTALLER_ROOT/LICENSE.txt" ]; then
    # Show first part of license
    head -60 "$INSTALLER_ROOT/LICENSE.txt"
    echo ""
    echo "... (Press Enter to see more, or type 'skip' to skip)"
    read -r response
    if [ "$response" != "skip" ]; then
        tail -n +61 "$INSTALLER_ROOT/LICENSE.txt"
    fi
else
    echo "DEMON SYNTH END USER LICENSE AGREEMENT"
    echo ""
    echo "Copyright (c) 2024 Nolan Griffis p/k/a Nully Beats"
    echo "Nully Beats LLC and Producer Tour Publishing LLC"
    echo "All Rights Reserved."
    echo ""
    echo "By installing this software, you agree to the terms of service"
    echo "available at https://nullybeats.com/terms"
fi

echo ""
echo "────────────────────────────────────────────────────────────────────"
echo ""
echo -e "${BOLD}Do you accept the terms of service?${NC}"
echo -e "Copyright © 2024 Nolan Griffis p/k/a Nully Beats"
echo -e "Nully Beats LLC and Producer Tour Publishing LLC"
echo ""

while true; do
    read -p "Type 'I AGREE' to accept and continue: " agreement
    if [ "$agreement" = "I AGREE" ] || [ "$agreement" = "i agree" ] || [ "$agreement" = "I agree" ]; then
        echo -e "${GREEN}✓ License agreement accepted${NC}"
        break
    else
        echo -e "${RED}You must type 'I AGREE' to continue with installation.${NC}"
    fi
done

echo ""
echo "════════════════════════════════════════════════════════════════════"
echo ""

# ============================================================================
# STEP 2: Plugin Installation Directories
# ============================================================================
echo -e "${YELLOW}${BOLD}STEP 2: Plugin Installation${NC}"
echo ""

# VST3 Path
echo -e "VST3 Plugin Location:"
echo -e "  Default: ${CYAN}$DEFAULT_VST3_PATH${NC}"
read -p "  Press Enter for default, or enter custom path: " custom_vst3
VST3_PATH="${custom_vst3:-$DEFAULT_VST3_PATH}"
echo -e "  ${GREEN}→ $VST3_PATH${NC}"
echo ""

# AU Path
echo -e "Audio Unit (AU) Location:"
echo -e "  Default: ${CYAN}$DEFAULT_AU_PATH${NC}"
read -p "  Press Enter for default, or enter custom path: " custom_au
AU_PATH="${custom_au:-$DEFAULT_AU_PATH}"
echo -e "  ${GREEN}→ $AU_PATH${NC}"
echo ""

# Standalone App Path
echo -e "Standalone Application Location:"
echo -e "  Default: ${CYAN}$DEFAULT_APP_PATH${NC}"
read -p "  Press Enter for default, or enter custom path: " custom_app
APP_PATH="${custom_app:-$DEFAULT_APP_PATH}"
echo -e "  ${GREEN}→ $APP_PATH${NC}"
echo ""

echo "════════════════════════════════════════════════════════════════════"
echo ""

# ============================================================================
# STEP 3: Sample Presets Directory
# ============================================================================
echo -e "${YELLOW}${BOLD}STEP 3: Sample Presets Location${NC}"
echo ""
echo -e "Choose where to install the sample presets."
echo -e "The plugin will look for samples in this directory."
echo ""
echo -e "  Default: ${CYAN}$DEFAULT_SAMPLES_PATH${NC}"
echo ""
read -p "  Press Enter for default, or enter custom path: " custom_samples
SAMPLES_PATH="${custom_samples:-$DEFAULT_SAMPLES_PATH}"
echo -e "  ${GREEN}→ $SAMPLES_PATH${NC}"
echo ""

echo "════════════════════════════════════════════════════════════════════"
echo ""

# ============================================================================
# STEP 4: Confirmation
# ============================================================================
echo -e "${YELLOW}${BOLD}STEP 4: Confirm Installation${NC}"
echo ""
echo "The following will be installed:"
echo ""
echo -e "  ${CYAN}VST3 Plugin:${NC}      $VST3_PATH/Demon Synth.vst3"
echo -e "  ${CYAN}AU Plugin:${NC}        $AU_PATH/Demon Synth.component"
echo -e "  ${CYAN}Standalone App:${NC}   $APP_PATH/Demon Synth.app"
echo -e "  ${CYAN}Sample Presets:${NC}   $SAMPLES_PATH"
echo ""

read -p "Proceed with installation? (y/n): " confirm
if [ "$confirm" != "y" ] && [ "$confirm" != "Y" ]; then
    echo -e "${RED}Installation cancelled.${NC}"
    exit 0
fi

echo ""
echo "════════════════════════════════════════════════════════════════════"
echo ""
echo -e "${YELLOW}Installing...${NC}"
echo ""

# ============================================================================
# INSTALLATION
# ============================================================================

# Create directories
mkdir -p "$VST3_PATH"
mkdir -p "$AU_PATH"
mkdir -p "$APP_PATH" 2>/dev/null || true
mkdir -p "$SAMPLES_PATH"
mkdir -p "$CONFIG_DIR"

# Install VST3
if [ -d "$INSTALLER_ROOT/Demon Synth.vst3" ]; then
    echo -n "  Installing VST3 Plugin... "
    rm -rf "$VST3_PATH/Demon Synth.vst3"
    cp -R "$INSTALLER_ROOT/Demon Synth.vst3" "$VST3_PATH/"
    xattr -rd com.apple.quarantine "$VST3_PATH/Demon Synth.vst3" 2>/dev/null || true
    echo -e "${GREEN}✓${NC}"
fi

# Install AU
if [ -d "$INSTALLER_ROOT/Demon Synth.component" ]; then
    echo -n "  Installing Audio Unit... "
    rm -rf "$AU_PATH/Demon Synth.component"
    cp -R "$INSTALLER_ROOT/Demon Synth.component" "$AU_PATH/"
    xattr -rd com.apple.quarantine "$AU_PATH/Demon Synth.component" 2>/dev/null || true
    # Reset AU cache
    killall -9 AudioComponentRegistrar 2>/dev/null || true
    echo -e "${GREEN}✓${NC}"
fi

# Install Standalone App
if [ -d "$INSTALLER_ROOT/Demon Synth.app" ]; then
    echo -n "  Installing Standalone App... "
    rm -rf "$APP_PATH/Demon Synth.app"
    cp -R "$INSTALLER_ROOT/Demon Synth.app" "$APP_PATH/"
    xattr -rd com.apple.quarantine "$APP_PATH/Demon Synth.app" 2>/dev/null || true
    echo -e "${GREEN}✓${NC}"
fi

# Install Samples
if [ -d "$INSTALLER_ROOT/Samples" ]; then
    echo -n "  Installing Sample Presets... "
    cp -R "$INSTALLER_ROOT/Samples/"* "$SAMPLES_PATH/" 2>/dev/null || true
    echo -e "${GREEN}✓${NC}"
fi

# Write config file with samples path
echo -n "  Writing configuration... "
cat > "$CONFIG_FILE" << EOF
{
    "version": "1.0.0",
    "samplesPath": "$SAMPLES_PATH",
    "installedDate": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")",
    "licenseAccepted": true
}
EOF
echo -e "${GREEN}✓${NC}"

echo ""
echo "════════════════════════════════════════════════════════════════════"
echo ""
echo -e "${GREEN}${BOLD}Installation Complete!${NC}"
echo ""
echo -e "Demon Synth has been successfully installed."
echo ""
echo -e "${YELLOW}Important:${NC}"
echo "  • Restart your DAW (FL Studio, Logic Pro, etc.) to see the plugin"
echo "  • If Gatekeeper blocks the plugin, go to:"
echo "    System Preferences → Security & Privacy → Allow"
echo ""
echo -e "Sample presets are located at:"
echo -e "  ${CYAN}$SAMPLES_PATH${NC}"
echo ""
echo -e "Thank you for choosing Demon Synth!"
echo -e "© 2024 Nully Beats LLC / Producer Tour Publishing LLC"
echo ""
read -p "Press Enter to close..."
