$ErrorActionPreference = "Stop"

& "$PSScriptRoot\package.ps1"
$root = Split-Path $PSScriptRoot -Parent
& "$root\build\gacha_core_tests.exe"
if ($LASTEXITCODE -ne 0) { throw "Core tests failed." }
& "$root\build\api_smoke_tests.exe"
if ($LASTEXITCODE -ne 0) { throw "API tests failed." }
& "$PSScriptRoot\test-desktop.ps1"
