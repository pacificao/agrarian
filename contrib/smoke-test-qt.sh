#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DATADIR="${DATADIR:-$(mktemp -d /tmp/agrarian-qt-smoke.XXXXXX)}"
LOGFILE="${LOGFILE:-/tmp/agrarian-qt-smoke.log}"
WAIT_SECONDS="${WAIT_SECONDS:-25}"
AGRARIAN_QT="${AGRARIAN_QT:-$ROOT/src/qt/agrarian-qt}"

require_path() {
  if [[ ! -x "$1" ]]; then
    echo "Missing executable: $1" >&2
    exit 1
  fi
}

require_path "$AGRARIAN_QT"
mkdir -p "$DATADIR"
rm -f "$LOGFILE"

set +e
timeout --kill-after=5s "${WAIT_SECONDS}s" xvfb-run -a "$AGRARIAN_QT" \
  -regtest -datadir="$DATADIR" -choosedatadir=0 -debug=qt -debug=rpc \
  -printtoconsole -server=0 -listen=0 -dnsseed=0 -connect=0 >"$LOGFILE" 2>&1
exit_code=$?
set -e

if [[ "$exit_code" -ne 124 ]]; then
  echo "Expected Qt wallet to keep running until timeout, got exit code $exit_code" >&2
  tail -n 80 "$LOGFILE" >&2 || true
  exit 1
fi

if ! grep -q "init message: Done loading" "$LOGFILE"; then
  echo "Qt wallet did not reach Done loading" >&2
  tail -n 120 "$LOGFILE" >&2 || true
  exit 1
fi

if grep -E "No functional TLS|No such signal|Unknown property" "$LOGFILE" >/dev/null; then
  echo "Qt wallet logged a Qt6 runtime compatibility warning" >&2
  grep -E "No functional TLS|No such signal|Unknown property" "$LOGFILE" >&2 || true
  exit 1
fi

echo "Agrarian Qt smoke test passed"
echo "  datadir: $DATADIR"
echo "  log: $LOGFILE"
