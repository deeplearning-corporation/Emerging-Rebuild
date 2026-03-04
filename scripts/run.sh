#!/bin/bash

echo "========================================"
echo "Run Emerging Program"
echo "========================================"
echo

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ -z "$1" ]; then
    read -p "请输入程序名 (不含 .emg): " PROG_NAME
else
    PROG_NAME="$1"
fi

if [ -z "$PROG_NAME" ]; then
    exit 1
fi

"$SCRIPT_DIR/build_program.sh" "$PROG_NAME"

if [ $? -ne 0 ]; then
    echo "[错误] 编译失败，无法运行"
    exit 1
fi

echo
echo "========================================"
echo "运行程序: $PROG_NAME"
echo "========================================"
echo

cd "$(dirname "$SCRIPT_DIR")"
"./$PROG_NAME"

echo
echo "========================================"
echo "程序运行完成"
echo "========================================"
echo