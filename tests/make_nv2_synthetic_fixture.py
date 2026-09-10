#!/usr/bin/env python3
"""Create non-proprietary NV2-shaped fixtures from confirmed public structure.
No chart geometry or copyrighted chart content is included.
"""
from pathlib import Path
import argparse, struct

MAGIC=0x00FE8050
SIG=0x081273AB

def tlv(tag: int, text: str) -> bytes:
    raw=text.encode('ascii')
    return struct.pack('<HH', tag, len(raw))+raw

def make(path: Path, corrupt=False):
    b=bytearray(61)
    struct.pack_into('<I',b,0,MAGIC if not corrupt else 0xDEADBEEF)
    struct.pack_into('<I',b,4,SIG)
    b[19]=0x20
    b[29:45]=b'2012120620121001'
    # Norfolk-ish synthetic extent in Web Mercator metres.
    struct.pack_into('<4i',b,45,18195902,-4007279,19286398,-2916825)
    b += tlv(9,'Marine e-chart')
    b += tlv(10,'TESTT32')
    b += tlv(11,'SYNTHETIC NV2 TEST')
    b += tlv(12,'Navionics')
    b += tlv(13,'Synthetic interoperability fixture; contains no chart data.')
    struct.pack_into('<I',b,11,len(b))
    path.write_bytes(b)

if __name__=='__main__':
    ap=argparse.ArgumentParser()
    ap.add_argument('output',type=Path)
    ap.add_argument('--corrupt',action='store_true')
    ns=ap.parse_args(); make(ns.output,ns.corrupt)
