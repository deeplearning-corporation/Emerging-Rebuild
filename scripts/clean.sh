#!/bin/bash

echo "========================================"
echo "Clean Emerging Build Files"
echo "========================================"
echo

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EMERGING_HOME="$(dirname "$SCRIPT_DIR")"

echo "清理目录: $EMERGING_HOME"
echo

find "$EMERGING_HOME" -type f -name "*.obj" -delete
find "$EMERGING_HOME" -type f -name "*.o" -delete
find "$EMERGING_HOME" -type f -name "*.exe" -delete
find "$EMERGING_HOME" -type f -name "emerging" -perm /111 -delete
find "$EMERGING_HOME" -type f -name "linker" -perm /111 -delete

echo "清理完成！"
echo