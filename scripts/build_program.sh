#!/bin/bash

echo "========================================"
echo "Emerging Program Builder"
echo "========================================"
echo

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EMERGING_HOME="$(dirname "$SCRIPT_DIR")"

if [ -z "$1" ]; then
    read -p "请输入程序名 (不含 .emg): " PROG_NAME
else
    PROG_NAME="$1"
fi

if [ -z "$PROG_NAME" ]; then
    exit 1
fi

SOURCE_FILE="$EMERGING_HOME/$PROG_NAME.emg"
OBJ_FILE="$EMERGING_HOME/$PROG_NAME.obj"
OUT_FILE="$EMERGING_HOME/$PROG_NAME"

if [ ! -f "$SOURCE_FILE" ]; then
    echo "[错误] 找不到源文件: $SOURCE_FILE"
    exit 1
fi

if [ ! -f "$EMERGING_HOME/emerging" ]; then
    echo "[错误] 找不到编译器，请先运行 build_tools.sh"
    exit 1
fi

if [ ! -f "$EMERGING_HOME/linker" ]; then
    echo "[错误] 找不到链接器，请先运行 build_tools.sh"
    exit 1
fi

echo "源文件: $SOURCE_FILE"
echo

echo "[1/2] 编译..."
cd "$EMERGING_HOME"
./emerging "$PROG_NAME.emg" "$PROG_NAME.obj"
if [ $? -ne 0 ]; then
    echo "[错误] 编译失败"
    exit 1
fi
echo "[完成]"

echo
echo "[2/2] 链接..."
if [[ "$(uname -s)" == "Linux" ]]; then
    DEFAULT_RUNTIME="lib/runtime/runtime_linux.obj"
elif [[ "$(uname -s)" == "Darwin" ]]; then
    DEFAULT_RUNTIME="lib/runtime/runtime_macos.obj"
else
    DEFAULT_RUNTIME="lib/runtime/runtime_linux.obj"
fi

read -p "请输入运行时库路径 (默认: $DEFAULT_RUNTIME): " RUNTIME_OBJ
if [ -z "$RUNTIME_OBJ" ]; then
    RUNTIME_OBJ="$DEFAULT_RUNTIME"
fi

./linker "$PROG_NAME" "$PROG_NAME.obj" "$RUNTIME_OBJ"
if [ $? -ne 0 ]; then
    echo "[错误] 链接失败"
    exit 1
fi
echo "[完成]"

echo
echo "========================================"
echo "编译链接成功！"
ls -l "$OUT_FILE"
echo "========================================"
echo