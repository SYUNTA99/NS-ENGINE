@echo off
cd /d "%~dp0.."
msbuild build\Mutra.sln -p:Configuration=Debug -p:Platform=x64 -m -v:normal
pause
