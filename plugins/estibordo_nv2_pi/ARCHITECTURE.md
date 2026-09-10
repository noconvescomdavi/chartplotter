# Estibordo NV2 chart plugin architecture

## Goal

Add legacy `.nv2` support to Estibordo Navigator without weakening OpenCPN's native chart engines and without redistributing or bypassing proprietary Navionics licensing components.

## Dual-backend design

### Backend A — `NavSdkBridge` (highest fidelity)

When the user already has a compatible, licensed ScanNav/Navionics runtime installed, Estibordo may dynamically load the user's `NavSDKDll.dll` and use the official interoperability surface. The bridge never ships the DLL, modifies activation data, or bypasses license checks.

The audited ScanNav64 package exposes at least these functional groups:

- lifecycle: `NavSDK_InitDll`, `NavSDK_CreateController`, `NavSDK_CloseController`
- charts: `NavSDK_MountChart`, `NavSDK_UnmountChart`, `NavSDK_UnmountAll`, `NavSDK_GetMountedCharts`
- georeferencing: `NavSDK_SetGeoPos`, `NavSDK_SetGeoRect`, `NavSDK_PositionToPix`, `NavSDK_PixToPosition`
- view: `NavSDK_ResizeView`, `NavSDK_Zoom`, `NavSDK_Scroll`
- rendering: `NavSDK_SetOnDrawCallBack`
- objects: `NavSDK_QueryObjects`, `NavSDK_GetObjectURI`, `NavSDK_GetObjectByURI`, `NavSDK_GetObjectAttributes`, object icons/photos/ports
- hydrography: depth units, tides and currents
- notifications/settings: `NavSDK_SetOnNotificationCallBack`, `NavSDK_OpenSettingsDial`

The bridge first performs export-surface validation. Calls into the ABI remain disabled until each parameter contract and ownership rule has been validated. This prevents crashes caused by guessing C ABI signatures.

### Backend B — `NV2Legacy` (independent reader)

For legacy `.nv2` datasets, Estibordo parses the file directly. The reader is bounded, little-endian, variant-aware and rejects unknown structures instead of guessing.

Confirmed across the current 11-file corpus:

- magic: `0x00FE8050`
- secondary signature: `0x081273AB`
- 32-bit declared file size at offset 11, matching physical file length in all tested charts
- 16-digit edition stamp at offset 29
- signed 32-bit Web Mercator extent at offsets 45/49/53/57
- stable top-level textual metadata containing `Marine e-chart`, chart id, chart title and vendor `Navionics`
- internal binary blocks after the fixed header; unknown blocks remain opaque until their structure is validated

## OpenCPN boundary

The final plugin advertises `INSTALLS_PLUGIN_CHART` and, after render parity, `INSTALLS_PLUGIN_CHART_GL`; provides a `.nv2` search mask; derives its chart class from `PlugInChartBase`; and exposes the confirmed extent through `GetChartExtent`.

The OpenCPN adapter uses one normalized feature model regardless of backend. When `NavSdkBridge` is active, rendering and object interrogation can be delegated to the licensed SDK. When `NV2Legacy` is active, decoded features are normalized into points, lines, areas, text and soundings before rendering.

## Components

1. `Nv2Reader`: bounded binary reader; no unchecked pointer arithmetic.
2. `Nv2Header`: signature, declared size, edition stamp, variant and Mercator extent.
3. `Nv2Metadata`: top-level chart identity and attribution.
4. `Nv2BlockIndex`: discovers and validates internal blocks; unknown block types remain opaque.
5. `Nv2FeatureDecoder`: converts validated legacy records into normalized geometry/features.
6. `Nv2FeatureStore`: immutable normalized model with source byte offsets and confidence state.
7. `NavSdkBridge`: optional Windows runtime adapter for user-owned licensed Navionics SDK components.
8. `Nv2Renderer`: CPU renderer first; GL renderer only after parity tests.
9. `Nv2Chart : PlugInChartBase`: OpenCPN chart adapter.
10. `Nv2Validator`: structural, geographic, topology and regression checks.
11. `nv2probe`: standalone NV2 validation/diagnostics tool.
12. `navsdkprobe`: standalone capability check for a local `NavSDKDll.dll`.

## Validation gates

- Gate A: file signature + exact declared length.
- Gate B: valid edition stamp + Mercator extent.
- Gate C: chart id/title/vendor metadata.
- Gate D: every indexed block stays inside file bounds; offsets monotonic where required.
- Gate E: decoded coordinates stay inside chart extent or documented tolerance.
- Gate F: line/area topology is internally consistent.
- Gate G: object classes, soundings, text and symbols match known-good reference behavior.
- Gate H: representative render snapshots match the same chart opened by a known-good viewer.
- Gate I: pixel-to-position and position-to-pixel round trips stay within defined tolerance.
- Gate J: unknown feature classes are reported, never guessed.

A chart may enter the OpenCPN chart database after Gate C. Full navigational rendering should require A-I for the detected decoder profile.

## Compatibility strategy

Compatibility is measured per NV2 variant, not by extension alone. The loader fingerprints structural fields and record/block signatures, then chooses:

1. official/user-installed `NavSdkBridge`, when available and licensed;
2. a validated `NV2Legacy` decoder profile;
3. diagnostics-only mode for unknown variants.

This architecture is the practical route to near-100% compatibility: official rendering for supported licensed datasets plus an independently testable fallback for legacy files. It does not claim universal NV2 support until the object/block grammar and render parity gates pass on a sufficiently broad corpus.
