#!/bin/bash
set -e

DEVKITPRO=${DEVKITPRO:-/opt/devkitpro}
DEVKITA64="$DEVKITPRO/devkitA64"
PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$PROJECT_DIR/build"

if [ ! -d "$DEVKITA64" ]; then
    echo "ERROR: DEVKITA64 not found at $DEVKITA64"
    echo "Install with: dkp-pacman -S switch-dev"
    exit 1
fi

echo "=============================================="
echo "  Greenlight Switch Builder"
echo "=============================================="
echo "DEVKITPRO : $DEVKITPRO"
echo "Project   : $PROJECT_DIR"
echo "Build dir : $BUILD_DIR"
echo ""
echo "Optional dependencies (enables features if present):"
echo "  switch-curl            — real Xbox API calls (HTTP)"
echo "  switch-sdl2            — graphical UI (Moonlight style)"
echo "  switch-sdl2_ttf        — text rendering in SDL2 UI"
echo "  switch-freetype        — font engine for SDL2_ttf"
echo ""
echo "  Install all: sudo dkp-pacman -S switch-curl switch-sdl2 switch-sdl2_ttf switch-freetype"
echo ""

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

export DEVKITPRO
export PATH="$DEVKITA64/bin:$PATH"

echo "[1/2] Configuring..."
cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" -DDEVKITPRO="$DEVKITPRO"

echo "[2/2] Building..."
DEVKITPRO="$DEVKITPRO" make -j"$(nproc)"

echo ""
echo "=============================================="
echo "  Build Complete"
echo "=============================================="
echo "  NRO: $BUILD_DIR/greenlight-switch.nro"
echo ""

# Copy NRO to dist/ for easy access
DIST_DIR="$PROJECT_DIR/dist"
mkdir -p "$DIST_DIR"
cp "$BUILD_DIR/greenlight-switch.nro" "$DIST_DIR/greenlight-switch.nro"
echo "  Copiado a: $DIST_DIR/greenlight-switch.nro"
echo ""
echo "Install: copy dist/greenlight-switch.nro to /switch/ on SD card"
echo ""
