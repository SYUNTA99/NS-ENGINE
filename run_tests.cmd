@echo off
setlocal enabledelayedexpansion

:: テストをビルドして実行するスクリプト
:: 使用方法: run_tests.cmd [Debug|Release]

set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Debug

echo ===================================
echo テストビルド・実行 (%CONFIG%)
echo ===================================

:: MSBuild パスを検索
set "MSBUILD="
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe 2^>nul`) do (
    set "MSBUILD=%%i"
)

if "%MSBUILD%"=="" (
    echo [ERROR] MSBuild が見つかりません
    exit /b 1
)

:: プロジェクト再生成
echo.
echo [1/3] プロジェクト生成中...
call tools\premake5.exe vs2022
if errorlevel 1 (
    echo [ERROR] Premake失敗
    exit /b 1
)

:: ビルド
echo.
echo [2/3] テストビルド中...
"%MSBUILD%" build\HEW2026.sln /t:tests /p:Configuration=%CONFIG% /p:Platform=x64 /m /v:minimal
if errorlevel 1 (
    echo [ERROR] ビルド失敗
    exit /b 1
)

:: テスト実行
echo.
echo [3/3] テスト実行中...
echo ===================================
set TEST_EXE=build\bin\%CONFIG%-windows-x86_64\tests\tests.exe
if not exist "%TEST_EXE%" (
    echo [ERROR] テスト実行ファイルが見つかりません: %TEST_EXE%
    exit /b 1
)

"%TEST_EXE%" --gtest_color=yes
set TEST_RESULT=%errorlevel%

echo.
if %TEST_RESULT%==0 (
    echo ===================================
    echo [OK] 全テスト成功
    echo ===================================
) else (
    echo ===================================
    echo [FAILED] テスト失敗
    echo ===================================
)

exit /b %TEST_RESULT%
