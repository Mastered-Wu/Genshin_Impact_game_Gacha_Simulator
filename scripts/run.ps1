$ErrorActionPreference = "Stop"

& "$PSScriptRoot\build.ps1"
& ".\build\gacha_server.exe"
