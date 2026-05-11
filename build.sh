#!/bin/bash

# Xbox xCloud Switch Builder
# Compila el proyecto y genera .nro

set -e

DEVKITPRO=${DEVKITPRO:-/opt/devkitpro}
DEVKITA64="$DEVKITPRO/devkitA64"
PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$PROJECT_DIR/build"

if [ ! -d "$DEVKITA64" ]; then
    echo "ERROR: DEVKITA64 not found at $DEVKITA64"
    echo "Please install DevKitPro with: dkp-pacman -S switch-dev"
    exit 1
fi

echo "=========================================="
echo "  Xbox xCloud Switch Builder"
echo "=========================================="
echo ""
echo "DEVKITPRO: $DEVKITPRO"
echo "DEVKITA64: $DEVKITA64"
echo "Project:   $PROJECT_DIR"
echo "Build dir: $BUILD_DIR"
echo ""

# Clean and create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Export toolchain
export DEVKITPRO
export PATH="$DEVKITA64/bin:$PATH"

# Configure
echo "[1/4] Configuring with CMake..."
cmake ..

# Build ELF
echo "[2/4] Compiling ELF..."
make

# Create NACP metadata
echo "[3/4] Creating metadata (control.nacp)..."
"$DEVKITPRO/tools/bin/nacptool" --create "xCloud Switch" "Jose Flores" "0.1" control.nacp --titleid=0x0100F00011E5C000

# Convert to NRO
echo "[4/4] Converting to NRO..."
"$DEVKITPRO/tools/bin/elf2nro" xcloud-switch.elf xcloud-switch.nro --nacp=control.nacp

# Success
echo ""
echo "=========================================="
echo "  ✅ Build Complete!"
echo "=========================================="
echo ""
echo "Output files:"
echo "  ELF:  $BUILD_DIR/xcloud-switch.elf"
echo "  NRO:  $BUILD_DIR/xcloud-switch.nro"
echo ""
echo "To install on Switch:"
echo "  1. Copy xcloud-switch.nro to /switch/ folder on SD card"
echo "  2. Run with HBL (Homebrew Launcher)"
echo ""
