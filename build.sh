#!/bin/bash

PROG="RegulaDocumentReader"
BUILD_DIR="dist"
CURRENT_DIR="$(realpath .)"

if [ -d "$BUILD_DIR" ];
then
  echo "Remove \"$BUILD_DIR\" folder"
  rm -rf "$PROG" "$BUILD_DIR"
fi

rm -rf "$CURRENT_DIR"/*.{bmp,jpg,xml,json}

cmake -S . -B dist && cd dist && make

echo "Create symlink on binary file"
cd .. && ln -s "${CURRENT_DIR}/${BUILD_DIR}/src/${PROG}" "${CURRENT_DIR}/${PROG}"
