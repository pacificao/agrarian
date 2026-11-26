#!/usr/bin/env bash
set -euo pipefail

package=${1:-/package/wallet.tar.gz}
scratch=$(mktemp -d)
chmod 755 "$scratch"
cd "$scratch"

. /etc/os-release
printf 'Testing on %s (%s); %s\n' "$PRETTY_NAME" "$(uname -m)" "$(getconf GNU_LIBC_VERSION)"
test "$(uname -m)" = x86_64
tar --no-same-owner -xzf "$package"

qt="$scratch/agrarian/bin/agrarian-qt"
daemon="$scratch/agrarian/bin/agrariand"
worker="$scratch/agrarian/bin/miners/agrarian-pool-worker"
for binary in "$qt" "$daemon" "$worker"; do
    test -x "$binary"
    dependencies=$(ldd "$binary")
    printf '%s\n' "$dependencies"
    if [[ $dependencies == *'not found'* ]]; then
        printf 'Missing library for %s\n' "$binary" >&2
        exit 1
    fi
done
test -s agrarian/bin/miners/COPYING
test -s agrarian/bin/miners/quark-screen.cl
test ! -e agrarian/bin/miners/cpuminer

runuser -u nobody -- "$daemon" --version
runuser -u nobody -- "$worker" --check-tls-backend
runuser -u nobody -- python3 /qa/first_party_pool_guard.py "$worker"
runuser -u nobody -- xvfb-run -a python3 /qa/desktop_package_smoke.py --qt "$qt" --daemon "$daemon"
