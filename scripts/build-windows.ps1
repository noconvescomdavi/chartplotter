param(
  [string]$Workspace = (Join-Path $PSScriptRoot "..\workspace"),
  [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$Workspace = [IO.Path]::GetFullPath($Workspace)
$Core = Join-Path $Workspace "OpenCPN"
$Build = Join-Path $Core "build-estibordo"

if (-not (Test-Path $Core)) { throw "Execute bootstrap.ps1 primeiro." }

New-Item -ItemType Directory -Force -Path $Build | Out-Null

cmake -S $Core -B $Build -G "Visual Studio 17 2022" -A x64 -DOCPN_USE_GL=ON -DOCPN_BUILD_TEST=OFF
if ($LASTEXITCODE -ne 0) { throw "Falha no configure CMake." }

cmake --build $Build --config $Configuration --parallel
if ($LASTEXITCODE -ne 0) { throw "Falha no build." }

Write-Host "Build concluído em $Build"
