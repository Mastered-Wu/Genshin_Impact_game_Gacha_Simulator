@echo off
start "" powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -Command "Set-Location -LiteralPath '%~dp0'; .\scripts\start-background.ps1; Start-Process 'http://127.0.0.1:18080/'"
