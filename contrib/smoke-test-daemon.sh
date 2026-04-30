#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DATADIR="${DATADIR:-$(mktemp -d /tmp/agrarian-smoke-regtest.XXXXXX)}"
RPCPORT="${RPCPORT:-36235}"
PORT="${PORT:-36236}"
BLOCKS="${BLOCKS:-1}"
WAIT_SECONDS="${WAIT_SECONDS:-90}"
AGRARIAND="${AGRARIAND:-$ROOT/src/agrariand}"
AGRARIAN_CLI="${AGRARIAN_CLI:-$ROOT/src/agrarian-cli}"

cleanup() {
  if [[ -x "$AGRARIAN_CLI" && -d "$DATADIR" ]]; then
    "$AGRARIAN_CLI" -datadir="$DATADIR" -regtest \
      -rpcuser=smoke -rpcpassword=smoke-pass -rpcport="$RPCPORT" stop >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

require_path() {
  if [[ ! -x "$1" ]]; then
    echo "Missing executable: $1" >&2
    exit 1
  fi
}

rpc() {
  "$AGRARIAN_CLI" -datadir="$DATADIR" -regtest \
    -rpcuser=smoke -rpcpassword=smoke-pass -rpcconnect=127.0.0.1 \
    -rpcport="$RPCPORT" "$@"
}

require_path "$AGRARIAND"
require_path "$AGRARIAN_CLI"

mkdir -p "$DATADIR"
cat > "$DATADIR/agrarian.conf" <<EOF
regtest=1
server=1
rpcuser=smoke
rpcpassword=smoke-pass
rpcbind=127.0.0.1
rpcallowip=127.0.0.1
listen=0
dnsseed=0
upnp=0
staking=0
keypool=1
EOF

"$AGRARIAND" -datadir="$DATADIR" -regtest -server -listen=0 -dnsseed=0 \
  -connect=0 -upnp=0 -staking=0 -rpcuser=smoke -rpcpassword=smoke-pass \
  -rpcbind=127.0.0.1 -rpcport="$RPCPORT" -port="$PORT" -keypool=1 -daemon

for _ in $(seq 1 "$WAIT_SECONDS"); do
  if rpc getwalletinfo >/dev/null 2>&1; then
    break
  fi
  sleep 1
done

if ! rpc getwalletinfo >/dev/null; then
  echo "Timed out waiting for wallet RPC on 127.0.0.1:$RPCPORT" >&2
  tail -n 80 "$DATADIR/regtest/debug.log" >&2 || true
  exit 1
fi
rpc getinfo >/dev/null
rpc getwalletinfo >/dev/null
rpc getmininginfo >/dev/null
rpc getstakingstatus >/dev/null
rpc getnewaddress smoke >/dev/null

before="$(rpc getblockcount)"
rpc generate "$BLOCKS" >/dev/null
after="$(rpc getblockcount)"

expected=$((before + BLOCKS))
if [[ "$after" -ne "$expected" ]]; then
  echo "Expected block count $expected after mining, got $after" >&2
  exit 1
fi

echo "Agrarian daemon smoke test passed"
echo "  datadir: $DATADIR"
echo "  blocks:  $before -> $after"
