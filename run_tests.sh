#!/bin/sh
set -e

BUILD_DIR="${1:-build-test}"

echo "==> Configuring..."
cmake -S "$(dirname "$0")" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_CLIENT=OFF \
    -DBUILD_SERVER=OFF \
    -DBUILD_GAME_LIBRARIES=OFF

echo "==> Building tests..."
cmake --build "$BUILD_DIR" --target test_md5 test_bans test_lz77 test_netsec 2>&1 || \
    cmake --build "$BUILD_DIR" 2>&1

echo ""
echo "==> Running tests..."
ctest --test-dir "$BUILD_DIR" --output-on-failure "$@"

echo ""
echo "==> All tests passed."
