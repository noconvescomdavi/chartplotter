#!/usr/bin/env python3
"""Validate a local NV2 corpus against tests/nv2-corpus-manifest.json.

No chart bytes leave the machine. The tool checks SHA-256/size and the stable
legacy NV2 header/TLV invariants used by the native decoder.
"""
from pathlib import Path
import argparse, hashlib, json, math, struct, sys

MAGIC=0x00FE8050
SIG=0x081273AB
R=6378137.0

def parse(path: Path):
    b=path.read_bytes()
    if len(b)<61: raise ValueError('short header')
    if struct.unpack_from('<I',b,0)[0]!=MAGIC: raise ValueError('magic')
    if struct.unpack_from('<I',b,4)[0]!=SIG: raise ValueError('signature')
    if struct.unpack_from('<I',b,11)[0]!=len(b): raise ValueError('declared size')
    stamp=b[29:45].decode('ascii')
    if len(stamp)!=16 or not stamp.isdigit(): raise ValueError('edition stamp')
    minx,miny,maxx,maxy=struct.unpack_from('<4i',b,45)
    marker=b.find(b'Marine e-chart',0,min(len(b),16384))
    if marker<4: raise ValueError('metadata marker')
    o=marker-4; md={}; dictionary=[]
    for _ in range(64):
        tag,l=struct.unpack_from('<HH',b,o)
        if tag==0x8030:
            q=b[o+4:o+4+l]; x=3
            for expected in range(19,29):
                t,n=struct.unpack_from('<HH',q,x)
                if t!=expected or x+4+n>len(q): raise ValueError('dictionary TLV')
                dictionary.append(q[x+4:x+4+n].decode('ascii'))
                x+=4+n
            break
        raw=b[o+4:o+4+l]
        if tag in (9,10,11,12): md[tag]=raw.decode('ascii')
        o+=4+l
    if md.get(9)!='Marine e-chart' or md.get(12)!='Navionics': raise ValueError('metadata')
    def inv(x,y):
        return math.degrees(x/R), math.degrees(2*math.atan(math.exp(y/R))-math.pi/2)
    sw=inv(minx,miny); ne=inv(maxx,maxy)
    return {'size':len(b),'sha256':hashlib.sha256(b).hexdigest(),'chart_id':md.get(10),'title':md.get(11),'vendor':md.get(12),'bbox':[round(sw[0],6),round(sw[1],6),round(ne[0],6),round(ne[1],6)],'dictionary_entries':dictionary}

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('directory',type=Path); ap.add_argument('--manifest',type=Path,default=Path('tests/nv2-corpus-manifest.json')); a=ap.parse_args()
    manifest=json.loads(a.manifest.read_text(encoding='utf-8'))
    failures=[]
    for expected in manifest['charts']:
        p=a.directory/expected['filename']
        if not p.exists(): failures.append(f"MISSING {p.name}"); continue
        try: got=parse(p)
        except Exception as e: failures.append(f"INVALID {p.name}: {e}"); continue
        keys=('size','sha256','chart_id','title','vendor','bbox')
        mismatch=[k for k in keys if got[k]!=expected[k]]
        if mismatch: failures.append(f"MISMATCH {p.name}: {','.join(mismatch)}")
        else: print(f"PASS {p.name}: {got['chart_id']} {got['title']} dict={len(got['dictionary_entries'])}")
    if failures:
        print('\n'.join(failures),file=sys.stderr); return 1
    print(f"PASS corpus: {len(manifest['charts'])}/{len(manifest['charts'])}")
    return 0
if __name__=='__main__': raise SystemExit(main())
