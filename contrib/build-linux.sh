#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
JOBS="${JOBS:-1}"
MODE="${MODE:-daemon}"
HOST="${HOST:-$("$ROOT/depends/config.guess")}"
PREFIX="$ROOT/depends/$HOST"
BASE_CONFIG="$PREFIX/share/config.site"

require_path() {
  if [[ ! -e "$1" ]]; then
    echo "Missing required path: $1" >&2
    exit 1
  fi
}

reset_configure_state() {
  rm -f config.cache config.log config.status

  # Stale generated makefiles can trigger config.status --recheck with old
  # configure arguments after a failed build attempt.
  find . -name Makefile -type f \
    ! -path './depends/*' \
    ! -path './.git/*' \
    -delete
}

cd "$ROOT"

case "$MODE" in
  daemon)
    echo "Building native daemon depends for $HOST..."
    make -C depends HOST="$HOST" NO_QT=1 -j"$JOBS"
    require_path "$BASE_CONFIG"

    reset_configure_state
    ./autogen.sh

    echo "Configuring Linux daemon build..."
    CONFIG_SITE="$BASE_CONFIG" ./configure \
      --without-gui \
      --disable-tests \
      --disable-bench \
      --disable-zmq \
      --with-miniupnpc=no \
      CXXFLAGS="${CXXFLAGS:--O0 -g0 --param ggc-min-expand=1 --param ggc-min-heapsize=32768}"
    ;;
  wallet)
    exec "$ROOT/contrib/build-linux-wallet.sh"
    ;;
  *)
    echo "Unknown MODE: $MODE" >&2
    echo "Use MODE=daemon or MODE=wallet." >&2
    exit 2
    ;;
esac

make -j"$JOBS"
echo "Linux $MODE build complete."
