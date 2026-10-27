"""User-run amd64 build for the identity change. Never opens a USB device."""
from pathlib import Path
import os
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
logs = root / 'build/torne-validation'
logs.mkdir(parents=True, exist_ok=True)
vswhere = Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)')) / 'Microsoft Visual Studio/Installer/vswhere.exe'
found = subprocess.check_output([
    str(vswhere), '-latest', '-products', '*', '-requires', 'Microsoft.Component.MSBuild',
    '-find', 'MSBuild/Current/Bin/MSBuild.exe']).decode('utf-8-sig').strip().splitlines()
if not found:
    raise SystemExit('MSBuild not found')

for name in ('torne_foundation_test', 'torne_usb_probe'):
    log = logs / (name + '-build.log')
    with log.open('wb') as output:
        result = subprocess.run([
            found[0], f'tests/{name}.vcxproj', '/t:Build',
            '/p:Configuration=Release-static;Platform=x64;PlatformToolset=v143',
            '/v:minimal', '/nologo'], cwd=root, stdout=output, stderr=subprocess.STDOUT)
    if result.returncode:
        print(f'FAIL: {name}; log={log}')
        print('\n'.join(log.read_text(encoding='utf-8-sig', errors='replace').splitlines()[-40:]))
        sys.exit(result.returncode)
    print(f'PASS: amd64 build {name}', flush=True)

log = logs / 'torne_foundation_test-run.log'
with log.open('wb') as output:
    result = subprocess.run([str(root/'build/x64/Release-static/torne_foundation_test.exe')],
                            cwd=root, stdout=output, stderr=subprocess.STDOUT, timeout=60)
print(log.read_text(encoding='utf-8-sig', errors='replace').strip())
if result.returncode:
    raise SystemExit(result.returncode)
print('PASS: identity build and offline tests. No USB device was opened.')
