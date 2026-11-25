$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
& "$PSScriptRoot\build.ps1"
New-Item -ItemType Directory -Path "$root\dist" -Force | Out-Null
Copy-Item -LiteralPath "$root\build\GachaSimulator.exe" -Destination "$root\dist\GachaSimulator.exe" -Force
Write-Host "Packaged: $root\dist\GachaSimulator.exe"
