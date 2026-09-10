param(
  [string]$HostAddress = "127.0.0.1",
  [int]$Port = 10110,
  [double]$Latitude = -22.9035,
  [double]$Longitude = -43.1729,
  [double]$Course = 90.0,
  [double]$SpeedKnots = 8.5
)

$ErrorActionPreference = "Stop"

function ToNmeaCoord([double]$value, [bool]$lat) {
  $abs = [Math]::Abs($value)
  $deg = [Math]::Floor($abs)
  $min = ($abs - $deg) * 60.0
  if ($lat) { return ("{0:00}{1:00.0000}" -f $deg, $min) }
  return ("{0:000}{1:00.0000}" -f $deg, $min)
}

function AddChecksum([string]$body) {
  $sum = 0
  foreach ($ch in $body.ToCharArray()) { $sum = $sum -bxor [int][char]$ch }
  return '$' + $body + '*' + ('{0:X2}' -f $sum)
}

$udp = New-Object System.Net.Sockets.UdpClient
$endpoint = New-Object System.Net.IPEndPoint([System.Net.IPAddress]::Parse($HostAddress), $Port)

Write-Host "Estibordo NMEA Simulator"
Write-Host "UDP -> $HostAddress`:$Port"
Write-Host "No Estibordo/OpenCPN: Options > Connections > Add Connection > Network > UDP > Port $Port"
Write-Host "Pressione Ctrl+C para encerrar."

$lat = $Latitude
$lon = $Longitude
while ($true) {
  $now = [DateTime]::UtcNow
  $time = $now.ToString("HHmmss.00")
  $date = $now.ToString("ddMMyy")
  $latHem = if ($lat -ge 0) { 'N' } else { 'S' }
  $lonHem = if ($lon -ge 0) { 'E' } else { 'W' }
  $latNmea = ToNmeaCoord $lat $true
  $lonNmea = ToNmeaCoord $lon $false

  $rmc = AddChecksum ("GPRMC,$time,A,$latNmea,$latHem,$lonNmea,$lonHem,{0:0.0},{1:0.0},$date,,,A" -f $SpeedKnots,$Course)
  $gga = AddChecksum ("GPGGA,$time,$latNmea,$latHem,$lonNmea,$lonHem,1,10,0.8,5.0,M,0.0,M,,")
  $vtg = AddChecksum ("GPVTG,{0:0.0},T,,M,{1:0.0},N,{2:0.0},K,A" -f $Course,$SpeedKnots,($SpeedKnots*1.852))

  foreach ($sentence in @($rmc,$gga,$vtg)) {
    $bytes = [Text.Encoding]::ASCII.GetBytes($sentence + "`r`n")
    [void]$udp.Send($bytes, $bytes.Length, $endpoint)
  }

  # Movimento simples para que track/COG/SOG sejam visíveis durante o teste.
  $distanceNm = $SpeedKnots / 3600.0
  $rad = $Course * [Math]::PI / 180.0
  $lat += ($distanceNm * [Math]::Cos($rad)) / 60.0
  $lon += ($distanceNm * [Math]::Sin($rad)) / (60.0 * [Math]::Cos($lat * [Math]::PI / 180.0))
  Start-Sleep -Seconds 1
}
