# Install prerequisites from docs/building.md first.
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $repoRoot
try {
    . ./tools/dev-env.ps1
    cmake --preset win-msvc
    if ($LASTEXITCODE -ne 0) { throw 'Configure failed' }
    cmake --build --preset win-msvc
    if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
    ctest --preset win-msvc
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed; packaging stopped' }
    cpack --config build/win-msvc/CPackConfig.cmake
    if ($LASTEXITCODE -ne 0) { throw 'Packaging failed' }
    Write-Host 'Packages: build/win-msvc/package'
} finally { Pop-Location }
