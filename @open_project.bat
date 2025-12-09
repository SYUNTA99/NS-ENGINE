@echo off
cd /d "%~dp0"

:: Generate project if needed
if not exist "build\HEW2026.sln" (
    echo Generating project...
    tools\premake5.exe vs2022
    if errorlevel 1 (
        echo [ERROR] Generation failed
        exit /b 1
    )
)

:: Build if needed
if not exist "bin\Debug-windows-x64\game\game.exe" (
    echo Building Debug...
    :: Use vswhere to find Visual Studio installation path
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "%VSWHERE%" (
        echo [ERROR] vswhere.exe not found. Please install Visual Studio 2022.
        exit /b 1
    )
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -requires Microsoft.Component.MSBuild -find Common7\Tools\VsDevCmd.bat`) do (
        set "VSCMD_PATH=%%i"
    )
    if not defined VSCMD_PATH (
        echo [ERROR] Visual Studio 2022 not found
        exit /b 1
    )
    call "%VSCMD_PATH%" -arch=amd64 >nul 2>&1
    msbuild build\HEW2026.sln -p:Configuration=Debug -p:Platform=x64 -m -v:minimal
    if errorlevel 1 (
        echo [ERROR] Build failed
        exit /b 1
    )
)

start "" "build\HEW2026.sln"
