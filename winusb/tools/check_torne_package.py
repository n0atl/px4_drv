"""Check architecture and required files of the normal CECH-ZD1J distribution."""
from pathlib import Path
import argparse
import struct

winusb=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser()
parser.add_argument('--dist',type=Path,default=winusb/'dist')
args=parser.parse_args()
for bits,expected in ((32,0x14c),(64,0x8664)):
    folder=args.dist/f'BonDriver_Torne_{bits}bit'
    if (folder/'TorneFrontend.exe').exists():raise SystemExit('Obsolete frontend helper: '+str(folder))
    for name in ('BonDriver_Torne'+'.dll','DriverHost_PX4.exe','WinSCard.dll'):
        with (folder/name).open('rb') as source:
            source.seek(0x3c);position=struct.unpack('<I',source.read(4))[0];source.seek(position+4)
            actual=struct.unpack('<H',source.read(2))[0]
        if actual!=expected: raise SystemExit('Incorrect architecture: '+str(folder/name))
    for name in ('BonDriver_Torne'+'.ini','BonDriver_PX4-T.ChSet.txt','DriverHost_PX4.ini','LICENSE'):
        if not (folder/name).is_file(): raise SystemExit('Missing: '+str(folder/name))
    ini=(folder/('BonDriver_Torne'+'.ini')).read_text(encoding='utf-8-sig')
    if 'Experimental' in ini or 'DriverHost_PX4.exe' not in ini: raise SystemExit('Wrong host configuration')
    if 'ChSetPath="BonDriver_PX4-T.ChSet.txt"' not in ini:raise SystemExit('Wrong channel table configuration')
    host=(folder/'DriverHost_PX4.exe').read_bytes()
    for encoding in ('ascii','utf-16-le'):
        if 'TorneFrontend.exe'.encode(encoding) in host:raise SystemExit('Host still references the external frontend helper')
    print('PASS: standard CECH-ZD1J '+str(bits)+'bit package')
if not (args.dist/'Driver/CECH-ZD1J.inf').is_file(): raise SystemExit('Missing driver INF')
for name in ('NOTICE','COPYING','LOCAL-CHANGES.md','upstream.json'):
    target=args.dist/'licenses/coax'/name
    if not target.is_file(): raise SystemExit('Missing: '+str(target))
    if target.read_bytes()!=(winusb/'tools/torne_frontend'/name).read_bytes():
        raise SystemExit('Incorrect license document: '+str(target))
print('PASS: shared coax license documents in dist/licenses/coax')
print('No DLL was loaded and no hardware was opened.')
