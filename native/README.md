# Estibordo Navigator Native

Este diretório é a nova base original do chartplotter.

## Regra arquitetural

O executável desta árvore **não incorpora código-fonte do OpenCPN**. OpenCPN é usado somente como referência funcional/comportamental e para comparação de interoperabilidade. O motor, UI, NMEA, rotas, adapters de cartas e renderização desta árvore são desenvolvidos independentemente.

## Estado atual

- aplicação Windows nativa C++20/Win32
- viewport próprio com pan e zoom
- barra MFD própria
- parser NMEA RMC/GGA próprio
- receptor UDP NMEA na porta 10110
- own-ship no canvas
- route mode com waypoints e linha de rota
- follow GPS
- arquitetura preparada para chart providers independentes

## Próximos chart providers

- KAP/BSB
- S-57 ENC
- MBTiles
- CM93 v2
- NV2 experimental
- outros providers via interface comum

## Build

Visual Studio 2022:

```powershell
cmake -S native -B native/build -G "Visual Studio 17 2022" -A x64
cmake --build native/build --config Release
```

O executável será gerado em `native/build/Release/EstibordoNavigator.exe`.
