param([string]$Workspace = (Join-Path $PSScriptRoot "..\workspace"))

$ErrorActionPreference = "Stop"
$Core = Join-Path ([IO.Path]::GetFullPath($Workspace)) "OpenCPN"
$Cache = Join-Path $Core "cache"
New-Item -ItemType Directory -Force -Path $Cache | Out-Null

function Download([string]$Url, [string]$Out) {
  if (Test-Path $Out) { return }
  Write-Host "Downloading $Url"
  Invoke-WebRequest -Uri $Url -OutFile $Out -UseBasicParsing
}

function Expand7z([string]$Archive, [string]$Dest) {
  & 7z x -y "-o$Dest" $Archive | Out-Host
  if ($LASTEXITCODE -ne 0) { throw "7z failed: $Archive" }
}

$wx = Join-Path $Cache "wxWidgets-3.2.9"
if (-not (Test-Path (Join-Path $wx "include\wx\wx.h"))) {
  New-Item -ItemType Directory -Force -Path $wx | Out-Null
  $base = "https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.9"
  $dev = Join-Path $Cache "wxMSW-3.2.9_vc14x_Dev.7z"
  $hdr = Join-Path $Cache "wxWidgets-3.2.9-headers.7z"
  $dll = Join-Path $Cache "wxMSW-3.2.9_vc14x_ReleaseDLL.7z"
  Download "$base/wxMSW-3.2.9_vc14x_Dev.7z" $dev
  Download "$base/wxWidgets-3.2.9-headers.7z" $hdr
  Download "$base/wxMSW-3.2.9_vc14x_ReleaseDLL.7z" $dll
  Expand7z $dev $wx
  Expand7z $hdr $wx
  Expand7z $dll $wx
}

$wxConfig = @"
set "wxWidgets_ROOT_DIR=$wx"
set "wxWidgets_LIB_DIR=$wx\lib\vc14x_dll"
"@
Set-Content (Join-Path $Cache "wx-config.bat") $wxConfig -Encoding ASCII

$buildwin = Join-Path $Cache "buildwin"
if (-not (Test-Path (Join-Path $buildwin "libcurl.dll"))) {
  $support = Join-Path $Cache "OCPNWindowsCoreBuildSupport.zip"
  Download "https://github.com/OpenCPN/OCPNWindowsCoreBuildSupport/archive/refs/tags/v0.5.zip" $support
  $tmp = Join-Path $Cache "buildwintemp"
  if (Test-Path $tmp) { Remove-Item $tmp -Recurse -Force }
  Expand-Archive $support -DestinationPath $tmp -Force
  New-Item -ItemType Directory -Force -Path $buildwin | Out-Null
  Copy-Item (Join-Path $tmp "OCPNWindowsCoreBuildSupport-0.5\buildwin\*") $buildwin -Recurse -Force
  $wxDllDir = Join-Path $buildwin "wxWidgets"
  if (Test-Path $wxDllDir) { Get-ChildItem $wxDllDir -Filter *.dll | Remove-Item -Force }
}

$iphlpapi = Join-Path $buildwin "iphlpapi.lib"
if (-not (Test-Path $iphlpapi)) {
  Download "https://dl.cloudsmith.io/public/alec-leamas/opencpn-support/raw/files/iphlpapi.lib" $iphlpapi
}

$localDocs = Join-Path $Core "data\doc\local"
New-Item -ItemType Directory -Force -Path $localDocs | Out-Null
$guide = Join-Path $Cache "QuickStartGuide.zip"
if (-not (Test-Path $guide)) {
  Download "https://dl.cloudsmith.io/public/david-register/opencpn-docs/raw/files/QuickStartGuide-v0.4.zip" $guide
}
Expand-Archive $guide -DestinationPath $localDocs -Force

Write-Host "Windows dependencies prepared without Chocolatey."
