"""Build/run the C++ port against independent Go I2C traces. No hardware access."""
from pathlib import Path
import argparse
import difflib
import os
import shutil
import subprocess
import sys

sys.stdout.reconfigure(encoding='utf-8')
winusb=Path(__file__).resolve().parents[1]
root=winusb.parent.parent
parser=argparse.ArgumentParser()
parser.add_argument('--arch',choices=('x86','x64'),default='x64')
parser.add_argument('--regenerate',action='store_true')
args=parser.parse_args()
build=winusb/'build/torne-native'/args.arch
build.mkdir(parents=True,exist_ok=True)
tool=winusb/'tools/torne_frontend'
go=shutil.which('go') or str(root/'.toolchains/go1.26.0/go/bin/go.exe')
env=os.environ.copy()
env.update(GOTOOLCHAIN='local',GOPROXY='off',GOCACHE=str(winusb/'build/torne_frontend/cache'),GOPATH=str(winusb/'build/torne_frontend/gopath'),CGO_ENABLED='0',TORNE_NATIVE_TRACE_DIR=str(build))
native=winusb/'src/DriverHost_PX4/torne'
if args.regenerate:
    generator=build/'port_native.go'
    shutil.copy2(tool/'port_native.go.txt',generator)
    subprocess.run([go,'run',str(generator),str(tool/'internal/coaxd/demod/dib8096'),str(native/'dib8096_native_generated.inc')],env=env,check=True)
subprocess.run([go,'test','-count=1','./internal/coaxd/demod/dib8096'],cwd=tool,env=env,check=True)
vswhere=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe')
vs=Path(subprocess.check_output([str(vswhere),'-latest','-property','installationPath'],text=True).strip())
version_file=vs/'VC/Auxiliary/Build/Microsoft.VCToolsVersion.v143.default.txt'
if not version_file.is_file():version_file=vs/'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt'
version=version_file.read_text().strip()
exe=build/'torne_native_frontend_test.exe'
batch=build/'build-test.cmd'
batch.write_text('@echo off\ncall "'+str(vs/'VC/Auxiliary/Build/vcvarsall.bat')+'" '+('amd64' if args.arch=='x64' else 'x86')+' -vcvars_ver='+version+'\nif errorlevel 1 exit /b 1\ncl /nologo /std:c++17 /EHsc /W3 /MT /utf-8 "'+str(native/'dib8096_native.cpp')+'" "'+str(winusb/'tests/torne_native_frontend_test.cpp')+'" /Fe"'+str(exe)+'"\n',encoding='utf-8')
log=build/'build-test.log'
with log.open('wb') as output:
    result=subprocess.run([r'C:\Windows\System32\cmd.exe','/d','/c',str(batch)],cwd=build,stdout=output,stderr=subprocess.STDOUT)
if result.returncode:
    print(log.read_text(encoding='cp932',errors='replace')[-14000:]);raise SystemExit(result.returncode)
subprocess.run([str(exe)],check=True,timeout=15)
failed=False
for mode in ('locked','nosignal','timeout','unknown','iofail','waitfail','cband'):
    target=build/(mode+'.cpp.trace')
    subprocess.run([str(exe),mode,str(target)],check=True,timeout=15)
    expected=(build/(mode+'.go.trace')).read_text().splitlines()
    actual=target.read_text().splitlines()
    if actual!=expected:
        failed=True
        print('FAIL:',args.arch,mode)
        print('\n'.join(list(difflib.unified_diff(expected,actual,fromfile='Go',tofile='C++'))[:75]))
    else:print('PASS:',args.arch,mode,len(actual),'identical I2C/wait/result events')
if failed:raise SystemExit(1)
print('PASS: native frontend comparison. No USB device was opened.')
