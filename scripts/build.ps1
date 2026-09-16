$ErrorActionPreference = "Stop"

$generator = "Ninja"
cmake -S backend -B build -G $generator
cmake --build build
