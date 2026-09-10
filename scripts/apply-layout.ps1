param([string]$Workspace = (Join-Path $PSScriptRoot "..\workspace"))

$ErrorActionPreference = "Stop"
$Core = Join-Path ([IO.Path]::GetFullPath($Workspace)) "OpenCPN"
$frame = Join-Path $Core "gui\src\ocpn_frame.cpp"
if (-not (Test-Path $frame)) { throw "OpenCPN core not found. Run bootstrap first." }

$text = Get-Content $frame -Raw
$old = 'toolbarParent, wxPoint(-1, -1), orient, g_toolbar_scalefactor,'
$new = 'toolbarParent, wxPoint(8, 72), wxTB_VERTICAL, g_toolbar_scalefactor * 1.15f,'
if ($text.Contains($old)) {
  $text = $text.Replace($old, $new)
} elseif (-not $text.Contains($new)) {
  throw "Main toolbar constructor pattern changed upstream; layout patch not applied."
}

# Use a slightly darker toolbar surface in every color scheme where the main
# toolbar is instantiated. This keeps contrast high over raster and ENC charts.
$text = $text.Replace('g_MainToolbar->SetBackGroundColorString("GREY3");',
                      'g_MainToolbar->SetBackGroundColorString("GREY2");')

Set-Content $frame $text -Encoding UTF8
Write-Host "Estibordo MFD layout applied: vertical left toolbar, enlarged controls."
