#!/usr/bin/env python3
from pathlib import Path
import argparse,struct
MAGIC=0x00FE8050; SIG=0x081273AB
def tlv(tag,text):
    raw=text.encode('ascii'); return struct.pack('<HH',tag,len(raw))+raw
def make(path,corrupt=False):
    b=bytearray(61); struct.pack_into('<I',b,0,MAGIC if not corrupt else 0xDEADBEEF); struct.pack_into('<I',b,4,SIG)
    b[19]=0x20; b[29:45]=b'2012120620121001'; struct.pack_into('<4i',b,45,18195902,-4007279,19286398,-2916825)
    b+=tlv(9,'Marine e-chart')+tlv(10,'TESTT32')+tlv(11,'SYNTHETIC NV2 TEST')+tlv(12,'Navionics')+tlv(13,'Synthetic interoperability fixture')
    b+=tlv(14,'SYN00001')+tlv(15,'TEST AREA')+tlv(16,'SYN00002')+tlv(17,'SECOND AREA')+tlv(18,'SYN00003')
    dictionary=b'\x00\x1c\x00'
    for tag,text in zip(range(19,29),['SYN001','AREA','SYN002','PORT','3','SYN003','LIGHT','W BN','FL W 5S','OCEAN']):
        dictionary+=tlv(tag,text)
    dictionary+=b'\x00'*64
    b+=struct.pack('<HH',0x8030,len(dictionary))+dictionary
    struct.pack_into('<I',b,11,len(b)); path.write_bytes(b)
if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('output',type=Path);ap.add_argument('--corrupt',action='store_true');n=ap.parse_args();make(n.output,n.corrupt)
