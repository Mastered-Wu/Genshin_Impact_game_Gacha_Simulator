@echo off
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\start-background.ps1"
if errorlevel 1 pause & exit /b 1
start "" "http://127.0.0.1:18080/"
