"""Generate the embedded MB86H47 programs without USB access or network use."""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--go', default='go', help='Go 1.26 or later executable')
parser.add_argument('--check', action='store_true', help='compare without replacing header')
args = parser.parse_args()
root = Path(__file__).resolve().parent
output = root.parents[1] / 'src/DriverHost_PX4/torne/mb86h47_programs.hpp'
build = root.parents[1] / 'build/torne_codegen'
build.mkdir(parents=True, exist_ok=True)
env = os.environ.copy()
env.update(GOTOOLCHAIN='local', GOPROXY='off', CGO_ENABLED='0', GOCACHE=str(build/'cache'), GOPATH=str(build/'gopath'), GOTMPDIR=str(build))
with tempfile.TemporaryDirectory(prefix='export-', dir=build) as temporary:
    generated = Path(temporary) / 'mb86h47_programs.hpp'
    env['TORNE_GENERATED_HEADER'] = str(generated)
    subprocess.run([args.go, 'test', '-count=1', './...'], cwd=root, env=env, check=True)
    content = generated.read_bytes()
    if args.check:
        if output.read_bytes().replace(b'\r\n', b'\n') != content:
            raise SystemExit('Generated MB86H47 programs differ')
        print('Generated MB86H47 programs match')
    else:
        output.write_bytes(content)
        print(output)
