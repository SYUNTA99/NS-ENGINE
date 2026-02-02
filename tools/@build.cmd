@echo off
::============================================================================
:: @build.cmd
:: プロジェクトをビルドするスクリプト
::
:: 使用方法: tools\@build.cmd [Debug|Release|Burst]
::============================================================================
call "%~dp0_common.cmd" :init

set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Debug

:: Regenerate project (fast, handles file add/delete)
call "%~dp0_common.cmd" :generate_project
if errorlevel 1 exit /b 1

:: Setup MSBuild
call "%~dp0_common.cmd" :setup_msbuild
if errorlevel 1 exit /b 1

echo %CONFIG% build...
msbuild build\NS-ENGINE.sln /p:Configuration=%CONFIG% /p:Platform=x64 /m /v:minimal
if errorlevel 1 (
    echo [ERROR] Build failed
    exit /b 1
)
echo [OK] Build success
