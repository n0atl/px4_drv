"""Build both Windows helper architectures for the normal DriverHost distribution."""
from pathlib import Path
import argparse
import shutil
import subprocess
import sys

here=Path(__file__).resolve().parent
winusb=here.parents[1]
parser=argparse.ArgumentParser()
parser.add_argument('--go')
args=parser.parse_args()
go=args.go or shutil.which('go')
if not go:
    local=here.parents[3]/'.toolchains/go1.26.0/go/bin/go.exe'
    if local.is_file(): go=str(local)
if not go: raise SystemExit('Go 1.26 or later required; set PATH or pass --go.')
for platform,arch in (('x86','386'),('x64','amd64')):
    output=winusb/'build'/platform/'Release-static/TorneFrontend.exe'
    subprocess.run([sys.executable,str(here/'build.py'),'--go',go,'--arch',arch,'--output',str(output)],check=True)
    print('PASS: release helper '+platform,flush=True)
