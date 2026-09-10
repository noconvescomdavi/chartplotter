#!/usr/bin/env python3
"""Estibordo Navigator NV2 reference decoder (read-only).

Confirmed across the current NV2 corpus:
- magic 0x00FE8050 at offset 0
- format signature 0x081273AB at offset 4
- declared file size at offset 11
- 16-byte ASCII edition/build stamp at offset 29
- four little-endian int32 Web Mercator bounds at offset 45
- metadata TLV records (u16 tag, u16 byte length, payload) containing
  format name, chart id, title, vendor and attribution.

Unknown object records are never assigned invented nautical semantics. Candidate
geometry is exported separately and marked EXPERIMENTAL until the record schema
is validated across multiple charts.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import struct
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Optional

MAGIC = 0x00FE8050
FORMAT_SIGNATURE = 0x081273AB
EARTH_RADIUS_M = 6378137.0

class NV2Error(Exception):
    pass

@dataclass
class MercatorBounds:
    min_x: int
    min_y: int
    max_x: int
    max_y: int

    def valid(self) -> bool:
        world = int(math.pi * EARTH_RADIUS_M) + 1
        return (self.min_x < self.max_x and self.min_y < self.max_y and
                all(-world <= v <= world for v in (self.min_x, self.min_y, self.max_x, self.max_y)))

    def to_wgs84(self) -> dict:
        west, south = mercator_to_lonlat(self.min_x, self.min_y)
        east, north = mercator_to_lonlat(self.max_x, self.max_y)
        return {"west": west, "south": south, "east": east, "north": north}

@dataclass
class Header:
    magic: int
    format_signature: int
    declared_size: int
    edition_stamp: str
    variant_byte: int
    bounds_mercator: MercatorBounds
    bounds_wgs84: dict

@dataclass
class MetadataRecord:
    tag: int
    offset: int
    length: int
    text: str

@dataclass
class Coord:
    offset: int
    x: int
    y: int
    lon: float
    lat: float

def mercator_to_lonlat(x: int, y: int) -> tuple[float, float]:
    lon = math.degrees(x / EARTH_RADIUS_M)
    lat = math.degrees(2.0 * math.atan(math.exp(y / EARTH_RADIUS_M)) - math.pi / 2.0)
    return lon, lat

def parse_header(data: bytes) -> Header:
    if len(data) < 61:
        raise NV2Error("file too short for NV2 header")
    magic = struct.unpack_from("<I", data, 0)[0]
    sig = struct.unpack_from("<I", data, 4)[0]
    declared = struct.unpack_from("<I", data, 11)[0]
    stamp = data[29:45].decode("ascii", errors="strict")
    vals = struct.unpack_from("<4i", data, 45)
    bounds = MercatorBounds(*vals)
    return Header(magic, sig, declared, stamp, data[19], bounds, bounds.to_wgs84())

def metadata_start(data: bytes) -> Optional[int]:
    marker = b"Marine e-chart"
    p = data.find(marker, 0, min(len(data), 16384))
    if p < 4:
        return None
    tag, length = struct.unpack_from("<HH", data, p - 4)
    return p - 4 if tag == 9 and length == len(marker) else None

def parse_metadata(data: bytes, scan_bytes: int = 512 * 1024) -> list[MetadataRecord]:
    start = metadata_start(data)
    if start is None:
        return []
    end = min(len(data), start + scan_bytes)
    out: list[MetadataRecord] = []
    o = start
    misses = 0
    last_good = o
    while o + 4 <= end:
        tag, length = struct.unpack_from("<HH", data, o)
        if 0 < length <= 4096 and o + 4 + length <= len(data):
            payload = data[o + 4:o + 4 + length]
            printable = all(c in (9, 10, 13) or 32 <= c < 127 for c in payload)
            if printable:
                out.append(MetadataRecord(tag, o, length, payload.decode("latin-1")))
                o += 4 + length
                last_good = o
                misses = 0
                continue
        o += 2
        misses += 1
        if misses > 4096 and o - last_good > 8192:
            break
    return out

def summarize_metadata(records: list[MetadataRecord]) -> dict:
    first: dict[int, str] = {}
    for r in records:
        first.setdefault(r.tag, r.text)
    return {
        "format": first.get(9), "chart_id": first.get(10), "title": first.get(11),
        "vendor": first.get(12), "attribution": first.get(13),
        "record_count": len(records), "records": [asdict(r) for r in records],
    }

def scan_coords(data: bytes, bounds: MercatorBounds) -> list[Coord]:
    if not bounds.valid():
        return []
    out: list[Coord] = []
    for o in range(61, len(data) - 7, 4):
        x, y = struct.unpack_from("<ii", data, o)
        if bounds.min_x <= x <= bounds.max_x and bounds.min_y <= y <= bounds.max_y:
            lon, lat = mercator_to_lonlat(x, y)
            if -180 <= lon <= 180 and -85.2 <= lat <= 85.2:
                out.append(Coord(o, x, y, lon, lat))
    return out

def group_runs(points: list[Coord], max_gap: int = 32, max_jump_deg: float = 5.0) -> list[list[Coord]]:
    if not points:
        return []
    runs: list[list[Coord]] = []
    cur = [points[0]]
    for p in points[1:]:
        q = cur[-1]
        if 0 < p.offset - q.offset <= max_gap and max(abs(p.lon - q.lon), abs(p.lat - q.lat)) <= max_jump_deg:
            cur.append(p)
        else:
            if len(cur) >= 2: runs.append(cur)
            cur = [p]
    if len(cur) >= 2: runs.append(cur)
    return runs

def decode(path: Path, include_geometry: bool = True) -> dict:
    data = path.read_bytes()
    h = parse_header(data)
    meta = summarize_metadata(parse_metadata(data))
    coords = scan_coords(data, h.bounds_mercator) if include_geometry else []
    runs = group_runs(coords) if include_geometry else []
    warnings: list[str] = []
    if h.magic != MAGIC: warnings.append(f"magic mismatch: 0x{h.magic:08X}")
    if h.format_signature != FORMAT_SIGNATURE: warnings.append(f"signature mismatch: 0x{h.format_signature:08X}")
    if h.declared_size != len(data): warnings.append("declared file size mismatch")
    if not re.fullmatch(r"\d{16}", h.edition_stamp): warnings.append("unexpected edition stamp")
    if not h.bounds_mercator.valid(): warnings.append("invalid Mercator extent")
    if meta.get("format") != "Marine e-chart": warnings.append("metadata marker missing")
    if meta.get("vendor", "").lower() != "navionics": warnings.append("unexpected vendor")
    return {
        "file": {"path": str(path), "size": len(data), "sha256": hashlib.sha256(data).hexdigest()},
        "header": {**asdict(h), "magic_hex": f"0x{h.magic:08X}", "format_signature_hex": f"0x{h.format_signature:08X}"},
        "metadata": meta,
        "validation": {"structural": "CONFIRMED" if not warnings else "PARTIAL", "geometry": "EXPERIMENTAL" if include_geometry else "NOT_SCANNED", "warnings": warnings},
        "geometry": {"candidate_count": len(coords), "run_count": len(runs), "runs": [[asdict(p) for p in run] for run in runs[:5000]]},
    }

def geojson(decoded: dict) -> dict:
    b = decoded["header"]["bounds_wgs84"]
    ring = [[b["west"],b["south"]],[b["east"],b["south"]],[b["east"],b["north"]],[b["west"],b["north"]],[b["west"],b["south"]]]
    features = [{"type":"Feature","geometry":{"type":"Polygon","coordinates":[ring]},"properties":{"kind":"NV2_CHART_EXTENT","confidence":"CONFIRMED"}}]
    for i, run in enumerate(decoded["geometry"]["runs"]):
        if len(run) >= 2:
            features.append({"type":"Feature","geometry":{"type":"LineString","coordinates":[[p["lon"],p["lat"]] for p in run]},"properties":{"kind":"NV2_COORDINATE_RUN","confidence":"EXPERIMENTAL","run":i,"offset_start":run[0]["offset"],"offset_end":run[-1]["offset"]}})
    return {"type":"FeatureCollection","features":features}

def main() -> int:
    ap = argparse.ArgumentParser(description="Read-only structural decoder/validator for legacy Navionics NV2 charts")
    ap.add_argument("files", nargs="+", type=Path)
    ap.add_argument("--json-dir", type=Path)
    ap.add_argument("--geojson-dir", type=Path)
    ap.add_argument("--no-geometry", action="store_true")
    ns = ap.parse_args()
    failures = 0
    for path in ns.files:
        try:
            d = decode(path, not ns.no_geometry)
            m, v = d["metadata"], d["validation"]
            print(f"{path.name}: {v['structural']} | {m.get('chart_id')} | {m.get('title')} | metadata={m['record_count']} | candidates={d['geometry']['candidate_count']}")
            if ns.json_dir:
                ns.json_dir.mkdir(parents=True, exist_ok=True)
                (ns.json_dir / f"{path.stem}.nv2.json").write_text(json.dumps(d, indent=2), encoding="utf-8")
            if ns.geojson_dir:
                ns.geojson_dir.mkdir(parents=True, exist_ok=True)
                (ns.geojson_dir / f"{path.stem}.geojson").write_text(json.dumps(geojson(d)), encoding="utf-8")
            if v["structural"] != "CONFIRMED": failures += 1
        except Exception as exc:
            failures += 1
            print(f"{path.name}: FAIL: {exc}")
    return 1 if failures else 0

if __name__ == "__main__":
    raise SystemExit(main())
