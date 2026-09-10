# Estibordo NV2 chart plugin architecture

## Goal

Add legacy `.nv2` support to Estibordo Navigator without contaminating or weakening OpenCPN's native chart engines.

## OpenCPN boundary

The plugin will advertise `INSTALLS_PLUGIN_CHART` and eventually `INSTALLS_PLUGIN_CHART_GL`, provide a `.nv2` search mask, derive its chart class from `PlugInChartBase`, and expose the confirmed NV2 extent through `GetChartExtent`. Rendering is enabled only after the object decoder validates the required feature classes for the detected NV2 variant.

## Components

1. `Nv2Reader`: bounded little-endian binary reader; no unchecked pointer arithmetic.
2. `Nv2Header`: magic, signature, declared size, edition stamp, variant and Web Mercator chart extent.
3. `Nv2Metadata`: TLV dictionary and chart identity.
4. `Nv2BlockIndex`: discovers and validates internal blocks; unknown block types remain opaque.
5. `Nv2FeatureDecoder`: converts validated NV2 records into normalized points/lines/areas/text/soundings.
6. `Nv2FeatureStore`: immutable normalized model with source byte offsets and confidence state.
7. `Nv2Renderer`: CPU renderer first, GL renderer after parity tests.
8. `Nv2Chart : PlugInChartBase`: OpenCPN adapter.
9. `Nv2Validator`: structural, geographic, topology and regression checks.

## Validation gates

- Gate A: file signature + exact declared length.
- Gate B: valid edition stamp + Mercator extent.
- Gate C: chart id/title/vendor metadata.
- Gate D: every block stays inside file bounds; offsets monotonic where required.
- Gate E: decoded coordinates stay inside chart extent or a documented tolerance.
- Gate F: line/area topology is internally consistent.
- Gate G: feature counts and representative render snapshots match a known-good reference for that chart variant.
- Gate H: unknown feature classes are reported, never guessed.

A chart may enter the OpenCPN chart database after Gate C. Full navigational rendering should require Gates A-G for the detected variant.

## Compatibility strategy

Compatibility is measured per NV2 variant, not by extension alone. The loader fingerprints structural fields and record/block signatures, then selects a decoder profile. Unknown profiles open in diagnostics mode and do not produce authoritative chart symbology.

This permits near-100% coverage of the tested legacy corpus while preventing one successful legacy variant from being incorrectly generalized to every Navionics generation.
