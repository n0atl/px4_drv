"""Build original-coax USB comparison, without changing its checkout."""
from pathlib import Path
import argparse
import os
import shutil
import subprocess
import tempfile

parser=argparse.ArgumentParser()
parser.add_argument('--go',default='go')
parser.add_argument('--coax',required=True)
args=parser.parse_args()
root=Path(__file__).resolve().parent
source=Path(args.coax).resolve()
revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=source,text=True).strip()
if revision!='2d9f2c263a3b080ecef3a8ede1eb86c6afa48405':raise SystemExit('Unexpected coax revision')
build=root.parents[1]/'build/torne_frontend'
build.mkdir(parents=True,exist_ok=True)
env=os.environ.copy()
env.update(GOTOOLCHAIN='local',GOPROXY='off',CGO_ENABLED='0',GOOS='windows',GOARCH='amd64',GOCACHE=str(build/'cache'),GOPATH=str(build/'gopath'),GOTMPDIR=str(build))
with tempfile.TemporaryDirectory(prefix='comparison-',dir=build) as directory:
 temporary=Path(directory).resolve()
 if not temporary.is_relative_to(build.resolve()):raise RuntimeError('Unexpected temporary path')
 for relative in ['internal/coaxd/demod/dib8096','internal/coaxd/usb/bridge/mb86h47','pkg/fr30','pkg/usbio','pkg/iso7816','pkg/i2c']:
  shutil.copytree(source/relative,temporary/relative)
 (temporary/'go.mod').write_text('module github.com/Till0196/coax\n\ngo 1.26.0\n',encoding='utf-8')
 command=temporary/'cmd/torne_compare'
 command.mkdir(parents=True)
 shutil.copy2(root/'reference_compare.go.txt',command/'main.go')
 output=root.parents[1]/'build/x64/Release-static/TorneOriginalBridgeCompare.exe'
 subprocess.run([args.go,'build','-trimpath','-o',str(output),'./cmd/torne_compare'],cwd=temporary,env=env,check=True)
 print(output)
