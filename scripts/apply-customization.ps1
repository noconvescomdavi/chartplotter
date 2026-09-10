param(
  [string]$Workspace = (Join-Path $PSScriptRoot "..\workspace"),
  [string]$BrandFile = (Join-Path $PSScriptRoot "..\config\brand.json")
)

$ErrorActionPreference = "Stop"
$Workspace = [IO.Path]::GetFullPath($Workspace)
$Core = Join-Path $Workspace "OpenCPN"
if (-not (Test-Path $Core)) { throw "Execute scripts\bootstrap.ps1 primeiro." }

$brand = Get-Content $BrandFile -Raw | ConvertFrom-Json
$product = $brand.productName
$company = $brand.companyName
$desc = $brand.description

function ReplaceLiteral($path, $old, $new) {
  if (-not (Test-Path $path)) { throw "Arquivo não encontrado: $path" }
  $txt = Get-Content $path -Raw
  if (-not $txt.Contains($old)) {
    Write-Warning "Trecho não encontrado em $path : $old"
    return
  }
  $txt = $txt.Replace($old, $new)
  Set-Content -Path $path -Value $txt -Encoding UTF8
}

$app = Join-Path $Core "gui\src\ocpn_app.cpp"
ReplaceLiteral $app 'MyApp::SetAppDisplayName("OpenCPN");' ('MyApp::SetAppDisplayName("' + $product + '");')
ReplaceLiteral $app 'wxString myframe_window_title = wxString("OpenCPN " + short_version_name);' ('wxString myframe_window_title = wxString("' + $product + ' " + short_version_name);')
ReplaceLiteral $app 'std::string title = _("Welcome to OpenCPN").ToStdString();' ('std::string title = _("Welcome to ' + $product + '").ToStdString();')

$rc = Join-Path $Core "resources\opencpn.rc.in"
ReplaceLiteral $rc 'VALUE "CompanyName",      "OpenCPN.org\0"' ('VALUE "CompanyName",      "' + $company + '\0"')
ReplaceLiteral $rc 'VALUE "FileDescription",  "Chart Plotter and Navigator\0"' ('VALUE "FileDescription",  "' + $desc + '\0"')
ReplaceLiteral $rc 'VALUE "ProductName",      "OpenCPN\0"' ('VALUE "ProductName",      "' + $product + '\0"')

$notice = @"
$product

Fork/customização do OpenCPN.
O núcleo derivado continua sujeito às licenças GPL aplicáveis.
Os avisos de copyright/licença do upstream devem ser preservados.
"@
Set-Content (Join-Path $Core "ESTIBORDO-NOTICE.txt") $notice -Encoding UTF8

Write-Host "Customização base aplicada: $product"
