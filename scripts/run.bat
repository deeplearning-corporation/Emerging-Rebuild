@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Run Emerging Program
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

call "%SCRIPTS_DIR%\build_program.bat" !PROG_NAME!

if errorlevel 1 (
    echo [错误] 编译失败，无法运行
    pause
    exit /b 1
)

echo.
echo ========================================
echo 运行程序: !PROG_NAME!.exe
echo ========================================
echo.

cd /d "%EMERGING_HOME%"
!PROG_NAME!.exe

echo.
echo ========================================
echo 程序运行完成
echo ========================================
echo.

pause