@echo off
chcp 65001 >nul
cd /d "%~dp0"
if not exist "build\HEW2026.sln" (
    echo Project not found. Run @make_project.cmd first.
    exit /b 1
)
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [ERROR] vswhere.exe not found. Please install Visual Studio 2022.
    exit /b 1
)
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -requires Microsoft.Component.MSBuild -find Common7\Tools\VsDevCmd.bat`) do (
    set "VSCMD_PATH=%%i"
)
if not defined VSCMD_PATH (
    echo [ERROR] Visual Studio not found
    exit /b 1
)
call "%VSCMD_PATH%" -arch=amd64 >nul 2>&1
echo Building Release...
msbuild build\HEW2026.sln -p:Configuration=Release -p:Platform=x64 -m -v:minimal
if errorlevel 1 (
    echo [ERROR] Build failed
) else (
    echo [OK] Build succeeded
)
