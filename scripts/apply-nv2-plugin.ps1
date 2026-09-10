param(
  [string]$Workspace = (Join-Path $PSScriptRoot "..\workspace")
)

$ErrorActionPreference = "Stop"
$Workspace = [IO.Path]::GetFullPath($Workspace)
$Core = Join-Path $Workspace "OpenCPN"
$Source = Join-Path $PSScriptRoot "..\plugins\estibordo_nv2_pi"
$Destination = Join-Path $Core "plugins\estibordo_nv2_pi"

if (-not (Test-Path $Core)) { throw "Execute scripts\bootstrap.ps1 primeiro." }
if (-not (Test-Path $Source)) { throw "Modulo NV2 nao encontrado: $Source" }

if (Test-Path $Destination) { Remove-Item $Destination -Recurse -Force }
Copy-Item $Source $Destination -Recurse -Force

Write-Host "Estibordo NV2 plugin staged at $Destination"
