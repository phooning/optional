#!/usr/bin/env sh
set -eu

BUILD_TYPE=Debug
BUILD_DIR="build/$BUILD_TYPE"

case "${1:-}" in
    --release)
        BUILD_TYPE=Release
    ;;
    --debug|"")
        BUILD_TYPE=Debug
    ;;
    *)
        echo "Usage: $0 [--debug|--release]"
        exit 1
    ;;
esac

: "${VCPKG_ROOT:?VCPKG_ROOT is not set. Example: export VCPKG_ROOT=\$HOME/dev/vcpkg}"

cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

ln -sf "$BUILD_DIR/compile_commands.json" compile_commands.json

cmake --build "$BUILD_DIR"

"$BUILD_DIR/optional"
