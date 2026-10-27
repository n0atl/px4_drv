"""Compare generated programs with the unmodified pinned coax runtime package."""
from pathlib import Path
import argparse
import os
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--go', default='go')
parser.add_argument('--coax', required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parent
upstream = Path(args.coax).resolve()
revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=upstream, text=True).strip()
if revision != '2d9f2c263a3b080ecef3a8ede1eb86c6afa48405':
    raise SystemExit('Unexpected coax revision')
build = root.parents[1] / 'build/torne_codegen'
build.mkdir(parents=True, exist_ok=True)
package = Path('internal/coaxd/usb/bridge/mb86h47')
with tempfile.TemporaryDirectory(prefix='upstream-', dir=build) as directory:
    temporary = Path(directory).resolve()
    if not temporary.is_relative_to(build.resolve()):
        raise RuntimeError('Unexpected temporary directory')
    for relative in [package, Path('pkg/fr30'), Path('pkg/usbio'), Path('pkg/i2c'), Path('pkg/iso7816')]:
        shutil.copytree(upstream / relative, temporary / relative)
    # The USB package does not import x/text; avoid the unrelated CLI dependency.
    (temporary / 'go.mod').write_text('module github.com/Till0196/coax\n\ngo 1.26.0\n', encoding='utf-8')
    shutil.copy2(root / package / 'export_test.go', temporary / package / 'torne_export_test.go')
    output = temporary / 'upstream.hpp'
    env = os.environ.copy()
    env.update(GOTOOLCHAIN='local', GOPROXY='off', CGO_ENABLED='0', GOCACHE=str(build/'cache'), GOPATH=str(build/'gopath'), GOTMPDIR=str(build), TORNE_GENERATED_HEADER=str(output))
    subprocess.run([args.go, 'test', '-count=1', '-run', '^TestExport$', './' + package.as_posix()], cwd=temporary, env=env, check=True)
    expected = root.parents[1] / 'src/DriverHost_PX4/torne/mb86h47_programs.hpp'
    if output.read_bytes() != expected.read_bytes().replace(b'\r\n', b'\n'):
        raise SystemExit('Original runtime and adapted generator outputs differ')
print('PASS: original coax runtime and adapted FR30 generator produce identical headers')
