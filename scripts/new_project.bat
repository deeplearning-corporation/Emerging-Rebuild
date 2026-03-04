@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Create New Emerging Project
echo ========================================
echo.

cd /d %~dp0
set SCRIPTS_DIR=%CD%
cd ..
set EMERGING_HOME=%CD%

if "%1"=="" (
    set /p PROJ_NAME="请输入项目名称: "
) else (
    set PROJ_NAME=%1
)

if "!PROJ_NAME!"=="" exit /b

set PROJ_DIR=%EMERGING_HOME%\!PROJ_NAME!

if exist "!PROJ_DIR!" (
    echo [警告] 目录已存在: !PROJ_DIR!
    set /p OVERWRITE="是否覆盖? (y/n): "
    if /i not "!OVERWRITE!"=="y" exit /b
) else (
    mkdir "!PROJ_DIR!"
)

cd /d "!PROJ_DIR!"

echo 创建项目: !PROJ_NAME!
echo 位置: !PROJ_DIR!
echo.

echo 选择项目类型:
echo 1 - Hello World
echo 2 - 空项目
echo 3 - 带标准库
echo.

set /p TYPE="请输入选择 (1-3): "

if "%TYPE%"=="1" (
    (
        echo use ^<std/io.emh^>
        echo.
        echo int main()
        echo {
        echo     println("Hello, World from Emerging!");
        echo     println("This is project: !PROJ_NAME!");
        echo     ret 0
        echo }
    ) > !PROJ_NAME!.emg
    echo 创建 Hello World 程序: !PROJ_NAME!.emg
)

if "%TYPE%"=="2" (
    (
        echo int main()
        echo {
        echo     ret 0
        echo }
    ) > !PROJ_NAME!.emg
    echo 创建空项目: !PROJ_NAME!.emg
)

if "%TYPE%"=="3" (
    (
        echo use ^<std/io.emh^>
        echo use ^<std/string.emh^>
        echo use ^<std/math.emh^>
        echo.
        echo int main()
        echo {
        echo     println("Standard Library Demo");
        echo     printf("The answer is: %d\n", 42);
        echo     ret 0
        echo }
    ) > !PROJ_NAME!.emg
    echo 创建标准库示例: !PROJ_NAME!.emg
)

echo.
echo ========================================
echo 项目创建成功！
echo 源文件: !PROJ_DIR!\!PROJ_NAME!.emg
echo.
echo 下一步: 运行 build_program.bat !PROJ_NAME!
echo ========================================
echo.

pause