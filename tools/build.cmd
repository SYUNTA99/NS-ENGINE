@echo off
call tools\_common.cmd :init

set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Debug

:: Regenerate project (fast, handles file add/delete)
call tools\_common.cmd :generate_project
if errorlevel 1 exit /b 1

:: Setup MSBuild
call tools\_common.cmd :setup_msbuild
if errorlevel 1 exit /b 1

echo %CONFIG% build...
msbuild build\NS-ENGINE.sln /p:Configuration=%CONFIG% /p:Platform=x64 /m /v:minimal
if errorlevel 1 (
    echo [ERROR] Build failed
    exit /b 1
)
echo [OK] Build success
