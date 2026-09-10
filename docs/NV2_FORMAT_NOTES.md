# NV2 reverse-engineering notes

Only fields validated across the current Estibordo NV2 corpus are recorded as confirmed. Unknown fields are intentionally not guessed.

## Confirmed fixed header layout

| Offset | Size | Encoding | Meaning |
|---:|---:|---|---|
| 0 | 4 | uint32 LE | Magic `0x00FE8050` |
| 4 | 4 | uint32 LE | Format signature `0x081273AB` |
| 11 | 4 | uint32 LE | Declared file size; equals physical byte length on every validated sample |
| 19 | 1 | byte | Variant byte; observed values differ between editions |
| 29 | 16 | ASCII | 16-digit edition/build stamp |
| 45 | 16 | 4 x int32 LE | Web Mercator extent: minX, minY, maxX, maxY in metres |

## Confirmed metadata dictionary

The first metadata records use `uint16 tag + uint16 payload_length + payload` little-endian framing.

- tag 9: `Marine e-chart`
- tag 10: chart identifier, for example `8U170T32`
- tag 11: chart title
- tag 12: vendor (`Navionics` in the corpus)
- tag 13: attribution/contact text

After the initial records, dictionary control/index records appear. Their exact framing is not certified yet, so the reference decoder resynchronizes conservatively instead of assigning semantics to unknown bytes.

## Confirmed projection

The four extent integers convert correctly as Web Mercator metres using earth radius 6378137 m. Cross-checking the current corpus produces geographically credible extents for Lake Kariba, Norfolk Island, Fiji/Tonga, Australia, Caribbean and eastern Pacific chart families.

## Geometry status

Little-endian signed 32-bit coordinate pairs inside the confirmed Mercator bounds occur repeatedly in later file regions. Many form spatially coherent runs. Exact object framing, feature classes, symbology, sounding/depth attributes and topology are still under reverse engineering. These coordinates remain `EXPERIMENTAL` until validated against multiple charts and a known-good reference renderer.

## Integration architecture

OpenCPN custom chart support is implemented through `PlugInChartBase`. NV2 should therefore stay isolated in a dedicated Estibordo plugin rather than modifying the native BSB/CM93/S-57 engines.

`NV2 -> signature/header validator -> metadata parser -> block/index decoder -> object decoder -> normalized feature model -> renderer -> PlugInChartBase`

Every decoded field/object must carry a state: `CONFIRMED`, `VALIDATED_VARIANT`, `EXPERIMENTAL` or `UNSUPPORTED`. Experimental geometry must never silently masquerade as certified hydrographic information.

## Safety/licensing boundary

The decoder is read-only and operates on chart files supplied by the user. It does not bypass activation, subscriptions, encryption, signatures, DRM or device binding. If a protected NV2 variant requires circumvention of an access-control mechanism, that mechanism remains intact; compatibility work is limited to data the software can legitimately read.
