# Estibordo Navigator

Chartplotter para Windows baseado no OpenCPN, com identidade e distribuição próprias, preservando o núcleo maduro de navegação.

## Recursos

- Cartas raster BSB/KAP e cartas vetoriais ENC/S-57 suportadas pelo core OpenCPN
- GPS/NMEA 0183 via serial/COM, TCP e UDP
- AIS e alarmes CPA/TCPA conforme suporte do core/plugins
- Waypoints, rotas e tracks
- Own-ship, COG, SOG, HDG e dados de navegação recebidos por NMEA
- OpenGL
- Importação/exportação GPX
- Plugins compatíveis com a API do OpenCPN
- Interface Estibordo com toolbar vertical lateral e controles ampliados
- Modo portátil e instalador Windows

## Executar

Baixe o artefato `Estibordo-Navigator-Windows` em GitHub Actions.

### Versão portátil

1. Extraia `Estibordo-Navigator-Portable.zip`.
2. Execute `INICIAR-ESTIBORDO.cmd`.
3. O primeiro início abre o assistente de configuração em modo portátil.

### Instalação

Execute `Estibordo-Navigator-Setup.exe` e siga o instalador.

## Adicionar cartas

Abra `Options > Charts`, adicione a pasta que contém as cartas e atualize/reconstrua o banco de cartas. Use somente cartas obtidas/licenciadas legalmente.

## GPS real

Abra `Options > Connections > Add Connection` e configure a fonte GPS/NMEA:

- Serial: selecione a porta COM do receptor
- TCP: informe host/porta
- UDP: informe a porta de escuta

## Teste GPS sem hardware

O pacote inclui `tools/nmea-simulator.ps1`.

Execute:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\nmea-simulator.ps1
```

Depois crie no Estibordo uma conexão UDP na porta `10110`. A posição simulada parte do Rio de Janeiro e se move continuamente, permitindo validar own-ship, COG, SOG e track.

## Teste de rota

Importe `samples/demo-route.gpx`. O arquivo contém waypoints e uma rota simples para validar importação e plotagem.

## Desenvolvimento

1. `scripts/bootstrap.ps1`
2. `scripts/apply-customization.ps1`
3. `scripts/apply-layout.ps1`
4. `scripts/build-windows.ps1`

O GitHub Actions reproduz esse fluxo e gera os distribuíveis Windows.

## Upstream

- OpenCPN/OpenCPN
- OpenCPN/plugins
- OpenCPN/opencpn-libs

## Licença

O código derivado do OpenCPN permanece sujeito às licenças GPL aplicáveis. Avisos de copyright e licença do upstream devem ser preservados. A identidade visual e os arquivos próprios deste repositório não removem nem substituem essas obrigações.

## Segurança de navegação

Este software é uma ferramenta auxiliar de navegação. Não substitui publicações náuticas oficiais, equipamentos obrigatórios, procedimentos de bordo, julgamento do navegante ou requisitos da autoridade marítima.
