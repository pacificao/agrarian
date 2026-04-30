#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DATADIR="${DATADIR:-$(mktemp -d /tmp/agrarian-wallet-smoke.XXXXXX)}"
RPCPORT="${RPCPORT:-36435}"
PORT="${PORT:-36436}"
WAIT_SECONDS="${WAIT_SECONDS:-90}"
AGRARIAND="${AGRARIAND:-$ROOT/src/agrariand}"
AGRARIAN_CLI="${AGRARIAN_CLI:-$ROOT/src/agrarian-cli}"
BACKUP="${BACKUP:-/tmp/agrarian-wallet-backup-smoke.dat}"

cleanup() {
  if [[ -x "$AGRARIAN_CLI" && -d "$DATADIR" ]]; then
    "$AGRARIAN_CLI" -datadir="$DATADIR" -regtest \
      -rpcuser=wallet -rpcpassword=wallet-pass -rpcport="$RPCPORT" stop >/dev/null 2>&1 || true
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
    -rpcuser=wallet -rpcpassword=wallet-pass -rpcconnect=127.0.0.1 \
    -rpcport="$RPCPORT" "$@"
}

require_path "$AGRARIAND"
require_path "$AGRARIAN_CLI"

mkdir -p "$DATADIR"
cat > "$DATADIR/agrarian.conf" <<EOF
regtest=1
server=1
rpcuser=wallet
rpcpassword=wallet-pass
rpcbind=127.0.0.1
rpcallowip=127.0.0.1
listen=0
dnsseed=0
upnp=0
staking=0
keypool=10
EOF

"$AGRARIAND" -datadir="$DATADIR" -regtest -server -listen=0 -dnsseed=0 \
  -connect=0 -upnp=0 -staking=0 -rpcuser=wallet -rpcpassword=wallet-pass \
  -rpcbind=127.0.0.1 -rpcport="$RPCPORT" -port="$PORT" -keypool=10 -daemon

for _ in $(seq 1 "$WAIT_SECONDS"); do
  if rpc getinfo >/dev/null 2>&1; then
    break
  fi
  sleep 1
done

if ! rpc getinfo >/dev/null; then
  echo "Timed out waiting for RPC on 127.0.0.1:$RPCPORT" >&2
  tail -n 80 "$DATADIR/regtest/debug.log" >&2 || true
  exit 1
fi

rpc getwalletinfo >/dev/null
receive_address="$(rpc getnewaddress receive)"
send_address="$(rpc getnewaddress sendtest)"
rpc generate 101 >/dev/null
mined_balance="$(rpc getbalance)"
txid="$(rpc sendtoaddress "$send_address" 1.0)"
rpc generate 1 >/dev/null
rpc gettransaction "$txid" >/dev/null

rm -f "$BACKUP"
rpc backupwallet "$BACKUP"
test -s "$BACKUP"

set +e
encrypt_output="$(rpc encryptwallet smoke-passphrase 2>&1)"
encrypt_code=$?
set -e
if [[ "$encrypt_code" -ne 0 ]]; then
  echo "$encrypt_output" >&2
  exit "$encrypt_code"
fi

sleep 2
if rpc getinfo >/dev/null 2>&1; then
  echo "Expected encryptwallet to stop the daemon, but RPC is still available" >&2
  exit 1
fi

echo "Agrarian wallet smoke test passed"
echo "  datadir: $DATADIR"
echo "  receive_address: $receive_address"
echo "  send_txid: $txid"
echo "  mined_balance_before_send: $mined_balance"
echo "  backup: $BACKUP"
