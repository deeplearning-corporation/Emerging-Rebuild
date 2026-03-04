@echo off
echo ========================================
echo Clean Emerging Build Files
echo ========================================
echo.

cd /d %~dp0
cd ..
set EMERGING_HOME=%CD%

echo 清理目录: %EMERGING_HOME%
echo.

del /s /q *.obj 2>nul
del /s /q *.exe 2>nul
del /s /q *.ilk 2>nul
del /s /q *.pdb 2>nul
del /s /q *.idb 2>nul
del /s /q *.pch 2>nul

echo 清理完成！
echo.

pause