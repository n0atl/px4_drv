"""Build amd64 components and run tests that never open USB devices."""
from pathlib import Path
import argparse
import os
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--msbuild', help='MSBuild.exe path; otherwise use vswhere')
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
msbuild = args.msbuild
if not msbuild:
    vswhere = Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)')) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    result = subprocess.run([str(vswhere), '-latest', '-products', '*', '-requires', 'Microsoft.Component.MSBuild', '-find', 'MSBuild/Current/Bin/MSBuild.exe'], capture_output=True, check=True)
    paths = result.stdout.decode('utf-8-sig').strip().splitlines()
    if not paths:
        raise SystemExit('MSBuild was not found')
    msbuild = paths[0]
log_dir = root / 'build/torne-validation'
log_dir.mkdir(parents=True, exist_ok=True)
projects = ['px4_winusb.sln'] + [f'tests/{name}.vcxproj' for name in [
    'torne_foundation_test', 'torne_bootstrap_test', 'ts_sync_condition_test', 'torne_usb_probe', 'torne_reference_client_test']]
for project in projects:
    log = log_dir / (Path(project).stem + '-build.log')
    with log.open('wb') as output:
        result = subprocess.run([msbuild, project, '/t:Build', '/p:Configuration=Release-static;Platform=x64;PlatformToolset=v143', '/v:minimal', '/nologo'], cwd=root, stdout=output, stderr=subprocess.STDOUT)
    if result.returncode:
        raise SystemExit(f'FAIL: build {project}; {log}')
    print(f'PASS: build {project}', flush=True)
tests = ['torne_foundation_test', 'torne_bootstrap_test', 'ts_sync_condition_test', 'smart_card_state_test', 'ringbuffer_purge_test']
if (root/'build/x64/Release-static/TorneFrontendDiagnostic.exe').is_file():
    tests.append('torne_reference_client_test')
else:
    print('SKIP: reference IPC test (build diagnostic helper first)', flush=True)
for name in tests:
    log = log_dir / (name + '-run.log')
    with log.open('wb') as output:
        try:
            result = subprocess.run([str(root / 'build/x64/Release-static' / (name + '.exe'))], cwd=root, stdout=output, stderr=subprocess.STDOUT, timeout=60)
        except subprocess.TimeoutExpired:
            raise SystemExit(f'FAIL: timeout {name}; {log}')
    if result.returncode:
        raise SystemExit(f'FAIL: {name}; {log}')
    print(log.read_text(encoding='utf-8', errors='replace').strip(), flush=True)
print('PASS: amd64 offline validation. USB probe was built but not executed.')
