"""Stage standard CECH-ZD1J BonDriver folders; never load DLLs or install a driver."""
from pathlib import Path
import argparse
import shutil

winusb=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser()
parser.add_argument('--dist',default=str(winusb/'dist'))
args=parser.parse_args()
dist=Path(args.dist).resolve()
if not dist.is_relative_to(winusb.resolve()): raise SystemExit('Distribution must stay inside winusb workspace')
# 旧版を手動で再パッケージした場合も、途中まで新バイナリーへ上書きしない。
for folder in dist.glob('BonDriver_*'):
    if (folder/'TorneFrontend.exe').exists():
        raise SystemExit('Use a fresh staging folder: obsolete frontend helper is present: '+str(folder))
for bits,platform in ((32,'x86'),(64,'x64')):
    folder=dist/f'BonDriver_Torne_{bits}bit'
    folder.mkdir(parents=True,exist_ok=True)
    binaries=winusb/'build'/platform/'Release-static'
    for source,name in (
        (binaries/'BonDriver_PX4.dll','BonDriver_Torne'+'.dll'),
        (binaries/'DriverHost_PX4.exe','DriverHost_PX4.exe'),
        (binaries/'WinSCard.dll','WinSCard.dll'),
        (winusb/'pkg/BonDriver_PX4'/('BonDriver_Torne'+'.ini'),'BonDriver_Torne'+'.ini'),
        (winusb/'pkg/BonDriver_PX4/BonDriver_PX4-T.ChSet.txt','BonDriver_PX4-T.ChSet.txt'),
        (winusb/'pkg/DriverHost_PX4/DriverHost_PX4.ini','DriverHost_PX4.ini')):
        shutil.copy2(source,folder/name)
    shutil.copy2(winusb.parent/'LICENSE',folder/'LICENSE')
    # torneには不要な旧梱包のIT930xファームウェアを除去する。
    firmware=folder/'it930x-firmware.bin'
    if firmware.is_file(): firmware.unlink()
# dist全体を配布単位として、共通ホストのライセンス文書を一か所に保持する。
license_folder=dist/'licenses/coax'
license_folder.mkdir(parents=True,exist_ok=True)
for name in ('COPYING','NOTICE','upstream.json','LOCAL-CHANGES.md'):
    shutil.copy2(winusb/'tools/torne_frontend'/name,license_folder/name)
for folder in dist.glob('BonDriver_*'):
    if not folder.is_dir(): continue
    # 旧配置は共通文書と一致する生成物のみ除去し、手動変更は保持する。
    for name in ('COPYING','NOTICE','upstream.json','LOCAL-CHANGES.md'):
        for legacy in (folder/('CECH-ZD1J.'+name),folder/'licenses/coax'/name):
            if legacy.is_file() and legacy.read_bytes()==(license_folder/name).read_bytes():
                legacy.unlink()
    for directory in (folder/'licenses/coax',folder/'licenses'):
        if directory.is_dir() and not any(directory.iterdir()): directory.rmdir()
print('Prepared standard CECH-ZD1J folders with in-process native frontend: '+str(dist))
