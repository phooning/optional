param(
    [ValidateSet("Debug", "Release")]
    [string]$BuildType = "Debug"
)

$ErrorActionPreference = "Stop"

if (-not $env:VCPKG_ROOT) {
    throw "VCPKG_ROOT is not set. Example: set it to C:\dev\vcpkg"
}

$BuildDir = "build\$BuildType"

cmake -S . -B $BuildDir `
    -DCMAKE_BUILD_TYPE=$BuildType `
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON `
    -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"

cmake --build $BuildDir --config $BuildType

$Exe1 = "$BuildDir\optional.exe"
$Exe2 = "$BuildDir\$BuildType\optional.exe"

if (Test-Path $Exe1) {
    & $Exe1
} elseif (Test-Path $Exe2) {
    & $Exe2
} else {
    throw "Could not find optional.exe in $BuildDir"
}