@echo off
cd /d "%~dp0"
echo Cleaning build artifacts...
if exist "build" rmdir /s /q "build" && echo   Removed: build/
if exist "bin" rmdir /s /q "bin" && echo   Removed: bin/
if exist "obj" rmdir /s /q "obj" && echo   Removed: obj/
echo [OK] Clean complete
