@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Emerging Program Builder
echo ========================================
echo.

cd /d %~dp0
set SCRIPTS_DIR=%CD%
cd ..
set EMERGING_HOME=%CD%

if "%1"=="" (
    set /p PROG_NAME="请输入程序名 (不含 .emg): "
) else (
    set PROG_NAME=%1
)

if "!PROG_NAME!"=="" exit /b

set SOURCE_FILE=%EMERGING_HOME%\!PROG_NAME!.emg
set OBJ_FILE=%EMERGING_HOME%\!PROG_NAME!.obj
set EXE_FILE=%EMERGING_HOME%\!PROG_NAME!.exe

if not exist "!SOURCE_FILE!" (
    echo [错误] 找不到源文件: !SOURCE_FILE!
    pause
    exit /b 1
)

if not exist "%EMERGING_HOME%\emerging.exe" (
    echo [错误] 找不到编译器，请先运行 build_tools.bat
    pause
    exit /b 1
)

if not exist "%EMERGING_HOME%\linker.exe" (
    echo [错误] 找不到链接器，请先运行 build_tools.bat
    pause
    exit /b 1
)

echo 源文件: !SOURCE_FILE!
echo.

echo [1/2] 编译...
cd /d "%EMERGING_HOME%"
emerging.exe !PROG_NAME!.emg !PROG_NAME!.obj
if errorlevel 1 (
    echo [错误] 编译失败
    pause
    exit /b 1
)
echo [完成]

echo.
echo [2/2] 链接...
set /p RUNTIME_OBJ="请输入运行时库路径 (默认: lib\runtime\runtime_win.obj): "
if "!RUNTIME_OBJ!"=="" set RUNTIME_OBJ=lib\runtime\runtime_win.obj

linker.exe !PROG_NAME!.exe !PROG_NAME!.obj !RUNTIME_OBJ!
if errorlevel 1 (
    echo [错误] 链接失败
    pause
    exit /b 1
)
echo [完成]

echo.
echo ========================================
echo 编译链接成功！
dir !EXE_FILE!
echo ========================================
echo.

pause