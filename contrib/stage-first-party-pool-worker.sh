#!/usr/bin/env bash
# Stage the compiled worker and its runtime kernel; never fetch executables.
set -euo pipefail
[[ $# == 2 ]] || { echo "Usage: $0 WORKER NEW_MINERS_DIRECTORY" >&2; exit 2; }
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
case "$(basename -- "$1")" in
    agrarian-pool-worker|agrarian-pool-worker.exe) ;;
    *) echo "Unexpected worker filename" >&2; exit 2 ;;
esac
[[ -f "$1" && ! -L "$1" ]]
mkdir -- "$2"
curl --proto '=https' --tlsv1.2 --fail --silent --show-error --max-time 120 \
    https://codeload.github.com/nicehash/sgminer/tar.gz/002021095feb64065c902863cf717af7f94854bd \
    --output "$2/quark-source.tar.gz"
PYTHONPATH="$root/test/functional" python3 -B - "$2" <<'PY'
import hashlib
from pathlib import Path
import sys

from opencl_quark_qa import source_from_archive

output = Path(sys.argv[1])
kernel = source_from_archive(output / "quark-source.tar.gz", True)
assert hashlib.sha256(kernel).hexdigest() == "9387b4e30715e7599843d24bc3bdded5921788d54f2e46dc5cddabf6bc8f5727"
(output / "quark-screen.cl").write_bytes(kernel)
PY
install -m 755 -- "$1" "$2/"
install -m 644 -- "$root/COPYING" "$2/COPYING"
