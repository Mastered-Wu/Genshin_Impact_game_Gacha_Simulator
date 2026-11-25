$ErrorActionPreference = "Stop"

$generator = "Ninja"
$root = Split-Path $PSScriptRoot -Parent
cmake -S "$root\backend" -B "$root\build" -G $generator
if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed." }
cmake --build "$root\build"
if ($LASTEXITCODE -ne 0) { throw "CMake build failed. Close the running EXE and retry." }
