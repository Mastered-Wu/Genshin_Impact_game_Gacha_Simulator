$ErrorActionPreference = "Stop"

$generator = "Ninja"
cmake -S backend -B build -G $generator
cmake --build build
& ".\build\gacha_core_tests.exe"
& ".\build\api_smoke_tests.exe"
