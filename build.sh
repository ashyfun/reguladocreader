#!/bin/bash

set -e

CURRENT_DIR="$(realpath .)"
BUILD_DIR="build"

PROG="RegulaDocumentReader"

BINARY="$CURRENT_DIR/$BUILD_DIR/src/$PROG"

rm -rf "$PROG" "$CURRENT_DIR/tmp"/*.{bmp,jpg,xml,json}
if [ -d "$BUILD_DIR" ]; then
  echo "Clear \"$BUILD_DIR\" directory"
  rm -rf "$BUILD_DIR"/*
fi

cmake -S . -B "$BUILD_DIR" && cd "$BUILD_DIR" && make
chmod +x "$BINARY"

echo "Create symlink on binary file"
cd .. && ln -s "$BINARY" "$CURRENT_DIR/$PROG"
