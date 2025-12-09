@echo off
setlocal
chcp 65001 >nul
cd /d "%~dp0"

echo Generating Visual Studio 2022 project...

:: テンポラリフォルダ用にGUIDを取得（日本語パス対策）
for /f %%a in ('powershell -command "$([guid]::NewGuid().ToString())"') do ( set GUID=%%a )

:: ジャンクション作成
mklink /j "%TEMP%\%GUID%" "%~dp0" > nul

:: Temp内からプロジェクト生成
pushd "%TEMP%\%GUID%"
    tools\premake5.exe vs2022
    set PREMAKE_RESULT=%errorlevel%
popd

:: ジャンクション削除
rmdir "%TEMP%\%GUID%"

if %PREMAKE_RESULT% neq 0 (
    echo [ERROR] Generation failed
    exit /b 1
)
echo [OK] build\HEW2026.sln generated
echo.
echo Building Debug...
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
msbuild build\HEW2026.sln -p:Configuration=Debug -p:Platform=x64 -m -v:minimal
if errorlevel 1 (
    echo [ERROR] Build failed
) else (
    echo [OK] Build succeeded
)
