@echo off
cd /d "%~dp0"

echo Current directory: %CD%
echo.

:: Generate project if needed
if not exist "build\HEW2026.sln" (
    echo Generating project...
    tools\premake5.exe vs2022
    if errorlevel 1 (
        echo [ERROR] Generation failed
        pause
        exit /b 1
    )
    echo [OK] Project generated
) else (
    echo [OK] build\HEW2026.sln exists
)

:: Build if needed
if not exist "bin\Debug-windows-x86_64\game\game.exe" (
    echo.
    echo Building Debug...

    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
)

if not exist "bin\Debug-windows-x86_64\game\game.exe" (
    if not exist "%VSWHERE%" (
        echo [ERROR] vswhere.exe not found. Please install Visual Studio 2022.
        pause
        exit /b 1
    )

    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
        set "MSBUILD_PATH=%%i"
    )
)

if not exist "bin\Debug-windows-x86_64\game\game.exe" (
    if not defined MSBUILD_PATH (
        echo [ERROR] MSBuild not found. Please install Visual Studio 2022 with C++ workload.
        pause
        exit /b 1
    )

    echo Using: %MSBUILD_PATH%
    "%MSBUILD_PATH%" build\HEW2026.sln -p:Configuration=Debug -p:Platform=x64 -m -v:minimal

    if errorlevel 1 (
        echo [ERROR] Build failed
        pause
        exit /b 1
    )
    echo [OK] Build succeeded
) else (
    echo [OK] game.exe already exists
)

echo.
echo Opening Visual Studio...
start "" "build\HEW2026.sln"

echo.
echo Done. Press any key to close...
pause
