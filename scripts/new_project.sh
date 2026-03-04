#!/bin/bash

echo "========================================"
echo "Create New Emerging Project"
echo "========================================"
echo

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EMERGING_HOME="$(dirname "$SCRIPT_DIR")"

if [ -z "$1" ]; then
    read -p "请输入项目名称: " PROJ_NAME
else
    PROJ_NAME="$1"
fi

if [ -z "$PROJ_NAME" ]; then
    exit 1
fi

PROJ_DIR="$EMERGING_HOME/$PROJ_NAME"

if [ -d "$PROJ_DIR" ]; then
    echo "[警告] 目录已存在: $PROJ_DIR"
    read -p "是否覆盖? (y/n): " OVERWRITE
    if [ "$OVERWRITE" != "y" ] && [ "$OVERWRITE" != "Y" ]; then
        exit 1
    fi
else
    mkdir -p "$PROJ_DIR"
fi

cd "$PROJ_DIR"

echo "创建项目: $PROJ_NAME"
echo "位置: $PROJ_DIR"
echo
echo "选择项目类型:"
echo "1 - Hello World"
echo "2 - 空项目"
echo "3 - 带标准库"
echo

read -p "请输入选择 (1-3): " TYPE

case "$TYPE" in
    1)
        cat > "$PROJ_NAME.emg" << EOF
use <std/io.emh>

int main()
{
    println("Hello, World from Emerging!");
    println("This is project: $PROJ_NAME!");
    ret 0
}
EOF
        echo "创建 Hello World 程序: $PROJ_NAME.emg"
        ;;
    2)
        cat > "$PROJ_NAME.emg" << EOF
int main()
{
    ret 0
}
EOF
        echo "创建空项目: $PROJ_NAME.emg"
        ;;
    3)
        cat > "$PROJ_NAME.emg" << EOF
use <std/io.emh>
use <std/string.emh>
use <std/math.emh>

int main()
{
    println("Standard Library Demo");
    printf("The answer is: %d\n", 42);
    ret 0
}
EOF
        echo "创建标准库示例: $PROJ_NAME.emg"
        ;;
    *)
        echo "无效选择"
        exit 1
        ;;
esac

echo
echo "========================================"
echo "项目创建成功！"
echo "源文件: $PROJ_DIR/$PROJ_NAME.emg"
echo
echo "下一步: 运行 ./scripts/build_program.sh $PROJ_NAME"
echo "========================================"
echo