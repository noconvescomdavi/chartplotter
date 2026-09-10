param([string]$Workspace = (Join-Path $PSScriptRoot "..\workspace"))

$ErrorActionPreference = "Stop"

function Require($name) {
  if (-not (Get-Command $name -ErrorAction SilentlyContinue)) {
    throw "Comando obrigatório não encontrado: $name"
  }
}

Require git
Require cmake

$Workspace = [IO.Path]::GetFullPath($Workspace)
New-Item -ItemType Directory -Force -Path $Workspace | Out-Null

function CloneOrUpdate($url, $dir, $branch) {
  if (Test-Path (Join-Path $dir ".git")) {
    git -C $dir fetch --all --tags
    git -C $dir checkout $branch
    git -C $dir pull --ff-only
  } else {
    git clone --branch $branch --depth 1 $url $dir
  }
  if ($LASTEXITCODE -ne 0) { throw "Falha ao preparar $url" }
}

CloneOrUpdate "https://github.com/OpenCPN/OpenCPN.git" (Join-Path $Workspace "OpenCPN") "master"
CloneOrUpdate "https://github.com/OpenCPN/plugins.git" (Join-Path $Workspace "plugins") "master"
CloneOrUpdate "https://github.com/OpenCPN/opencpn-libs.git" (Join-Path $Workspace "opencpn-libs") "main"

Write-Host "Upstreams preparados em $Workspace"
