#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DATADIR="${DATADIR:-$(mktemp -d /tmp/agrarian-wallet-smoke.XXXXXX)}"
RPCPORT="${RPCPORT:-36435}"
PORT="${PORT:-36436}"
WAIT_SECONDS="${WAIT_SECONDS:-90}"
AGRARIAND="${AGRARIAND:-$ROOT/src/agrariand}"
AGRARIAN_CLI="${AGRARIAN_CLI:-$ROOT/src/agrarian-cli}"
BACKUP="${BACKUP:-$DATADIR/wallet-backup-smoke.dat}"
STARTED=0

cleanup() {
  if [[ "$STARTED" == 1 ]]; then
    rpc stop >/dev/null 2>&1 || true
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
    -rpcconnect=127.0.0.1 -rpcport="$RPCPORT" "$@"
}

require_path "$AGRARIAND"
require_path "$AGRARIAN_CLI"

if [[ -L "$DATADIR" ]]; then
  echo "The smoke-test datadir must not be a symlink." >&2
  exit 1
fi
mkdir -p -- "$DATADIR"
test_dir="$(realpath -e -- "$DATADIR")"
dir_contents="$(find "$test_dir" -mindepth 1 -maxdepth 1 -print -quit)"
if [[ -n "$dir_contents" ]]; then
  echo "A fresh, empty smoke-test datadir is required." >&2
  exit 1
fi
if [[ -e "$BACKUP" || -L "$BACKUP" ]]; then
  echo "Refusing to overwrite an existing backup: $BACKUP" >&2
  exit 1
fi
backup_dir="$(realpath -e -- "$(dirname -- "$BACKUP")")"
if [[ "$backup_dir" != "$test_dir" ]]; then
  echo "The smoke-test backup must be inside its disposable datadir." >&2
  exit 1
fi

smoke_rpc_password="$(openssl rand -hex 32)"
(
  umask 077
  set -o noclobber
  cat > "$DATADIR/agrarian.conf" <<EOF
regtest=1
server=1
rpcuser=wallet
rpcpassword=$smoke_rpc_password
rpcbind=127.0.0.1
rpcallowip=127.0.0.1
listen=0
dnsseed=0
upnp=0
staking=0
keypool=10
EOF
)

"$AGRARIAND" -datadir="$DATADIR" -regtest -server -listen=0 -dnsseed=0 \
  -connect=0 -upnp=0 -staking=0 -rpcbind=127.0.0.1 \
  -rpcport="$RPCPORT" -port="$PORT" -keypool=10 -daemon
STARTED=1

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
