"""User-run build only: never loads the DLL, starts a host, or opens a tuner."""
from pathlib import Path
import os
import subprocess
import sys

here=Path(__file__).resolve().parent
root=here.parents[1]
vswhere=Path(os.environ.get('ProgramFiles(x86)','C:/Program Files (x86)'))/'Microsoft Visual Studio/Installer/vswhere.exe'
found=subprocess.check_output([str(vswhere),'-latest','-products','*','-requires','Microsoft.Component.MSBuild','-find','MSBuild/Current/Bin/MSBuild.exe']).decode('utf-8-sig').strip().splitlines()
if not found: raise SystemExit('MSBuild not found')
log=root/'build/torne-experimental/bondriver-probe-build.log'
log.parent.mkdir(parents=True,exist_ok=True)
with log.open('wb') as output:
    result=subprocess.run([found[0],str(here/'bondriver_probe.vcxproj'),'/t:Build',
        '/p:Configuration=Release-static;Platform=x64;PlatformToolset=v143','/v:minimal','/nologo'],
        cwd=root,stdout=output,stderr=subprocess.STDOUT)
if result.returncode:
    print(f'FAIL: probe build; log={log}')
    print('\n'.join(log.read_text(encoding='utf-8-sig',errors='replace').splitlines()[-40:]))
    sys.exit(result.returncode)
print('PASS: amd64 BonDriver probe build. No DLL was loaded and no tuner was opened.')
