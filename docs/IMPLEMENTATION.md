# Estibordo Navigator — Implementation Matrix

## Fase 1 — Fork e build Windows
Status: implementado. Bootstrap reproduzível com OpenCPN upstream e build Windows via GitHub Actions.

## Fase 2 — Identidade própria
Status: implementado. Nome da aplicação, título da janela, metadados do executável e distribuição Estibordo.

## Fase 3 — Layout próprio
Status: implementado na primeira versão MFD: toolbar principal lateral vertical, posição fixa inicial, controles ampliados e superfície de alto contraste. Evoluções visuais futuras não alteram o motor.

## Fase 4 — Chart engine
Status: herdado e preservado do OpenCPN. Raster/BSB/KAP e ENC/S-57 conforme suporte do core.

## Fase 5 — GPS / NMEA
Status: herdado e preservado. Serial/COM, TCP e UDP. Simulador UDP incluído para validação sem hardware.

## Fase 6 — Own ship
Status: herdado e preservado. Posição, COG/SOG e acompanhamento conforme dados NMEA disponíveis.

## Fase 7 — Waypoints / Routes / Tracks
Status: herdado e preservado. GPX de demonstração incluído no pacote.

## Fase 8 — AIS
Status: herdado e preservado do OpenCPN. A validação com receptor físico depende do equipamento do usuário.

## Fase 9 — Dashboard / Instruments
Status: instrumentos e dados do core preservados; plugins adicionais podem ser instalados usando o ecossistema OpenCPN.

## Fase 10 — Plugin compatibility
Status: preservada por manter o core e a API do OpenCPN sem reescrita.

## Fase 11 — GRIB / Weather
Status: disponível por plugins compatíveis do ecossistema OpenCPN. Não são embutidos no core para evitar acoplamento de versões.

## Fase 12 — Touch / Fullscreen
Status: controles ampliados no layout Estibordo e suporte de fullscreen herdado do OpenCPN.

## Fase 13 — Performance Windows
Status: build Release com OpenGL e dependências oficiais do OpenCPN.

## Fase 14 — Installer / Portable
Status: NSIS + ZIP portátil gerados pelo CI.

## Fase 15 — Testes
Status: smoke test Windows automatizado valida carregamento do executável e presença das rotinas CLI de banco de cartas, ENC/S-57 e GPX. GPS é validável com o simulador incluído. Testes finais com GPS/AIS físicos e cartas licenciadas devem ser feitos no equipamento de navegação onde o software será usado.

## Critério de release
Uma build só é distribuída quando compila, gera portátil e instalador e passa o smoke test do runtime no runner Windows.
