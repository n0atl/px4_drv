"""Build the isolated diagnostic helper and run its original frontend tests."""
from pathlib import Path
import argparse
import os
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--go', default='go')
parser.add_argument('--arch',choices=('amd64','386'),default='amd64')
parser.add_argument('--output',help='explicit output executable path')
args = parser.parse_args()
root = Path(__file__).resolve().parent
build = root.parents[1] / 'build/torne_frontend'
build.mkdir(parents=True, exist_ok=True)
output = Path(args.output).resolve() if args.output else root.parents[1] / 'build/x64/Release-static/TorneFrontendDiagnostic.exe'
output.parent.mkdir(parents=True, exist_ok=True)
env = os.environ.copy()
env.update(GOTOOLCHAIN='local', GOPROXY='off', CGO_ENABLED='0', GOOS='windows', GOARCH=args.arch, GOCACHE=str(build/'cache'), GOPATH=str(build/'gopath'), GOTMPDIR=str(build))
subprocess.run([args.go, 'test', '-count=1', './...'], cwd=root, env=env, check=True)
subprocess.run([args.go, 'build', '-trimpath', '-o', str(output), './cmd/torne_frontend'], cwd=root, env=env, check=True)
# Go's runtime is statically linked and its license must accompany distribution.
goroot = subprocess.check_output([args.go, 'env', 'GOROOT'], cwd=root, env=env, text=True).strip()
(root/'GO-LICENSE').write_bytes((Path(goroot)/'LICENSE').read_bytes())
print(output)
