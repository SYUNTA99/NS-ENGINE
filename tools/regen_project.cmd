@echo off
chcp 65001 >nul
cd /d "%~dp0"
call tools\_common.cmd :generate_project
pause
