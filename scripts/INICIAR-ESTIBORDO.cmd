@echo off
setlocal
cd /d "%~dp0"
if exist "opencpn.exe" (
  start "Estibordo Navigator" "opencpn.exe" -p -W
  exit /b 0
)
if exist "bin\opencpn.exe" (
  cd /d "%~dp0bin"
  start "Estibordo Navigator" "opencpn.exe" -p -W
  exit /b 0
)
echo Nao foi possivel localizar opencpn.exe neste pacote.
pause
exit /b 1
