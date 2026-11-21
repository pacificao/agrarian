#!/usr/bin/env bash
# Build the optional Linux desktop pool helper from its pinned source.
set -euo pipefail

if [[ $# != 1 || ! -d "$1" ]]; then
    printf 'Usage: %s LINUX_WALLET_BIN_DIRECTORY\n' "$0" >&2
    exit 2
fi
wallet_bin=$(realpath -e -- "$1")
[[ "$wallet_bin" != / && -f "$wallet_bin/agrarian-qt" && -x "$wallet_bin/agrarian-qt" ]] || {
    printf 'Expected an existing staging directory containing agrarian-qt.\n' >&2
    exit 2
}
[[ ! -e "$wallet_bin/miners" && ! -L "$wallet_bin/miners" ]] || {
    printf 'Not overwriting an existing miners directory.\n' >&2
    exit 2
}

revision=d2927ed23b1d0eacd067c320fce64e6610737adb
checksum=3ab898b2f1c87414e3ec16df7f0a0b53f82d5b11930852c963812c5bd9ff4d48
jobs=${JOBS:-2}
[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || exit 2
work=$(mktemp -d "${TMPDIR:-/tmp}/agrarian-miner.XXXXXXXX")
[[ -d "$work" && ! -L "$work" ]]
printf 'Building in %s\n' "$work"
source_tar="$work/cpuminer-multi-$revision.tar.gz"
curl --fail --silent --show-error --location --connect-timeout 10 --max-time 90 \
    --retry 3 --output "$source_tar" \
    "https://codeload.github.com/tpruvot/cpuminer-multi/tar.gz/$revision"
printf '%s  %s\n' "$checksum" "$source_tar" | sha256sum -c --strict
mkdir "$work/source"
tar --extract --gzip --no-same-owner --strip-components=1 --file "$source_tar" --directory "$work/source"
(
    cd "$work/source"
    bash ./autogen.sh
    CFLAGS="-O2 -fcommon -std=gnu11" ./configure --disable-assembly --with-crypto=/usr --with-curl=/usr
    make -j"$jobs"
    ./cpuminer --version
)

mkdir "$wallet_bin/miners"
install -m 0755 "$work/source/cpuminer" "$wallet_bin/miners/"
install -m 0644 "$work/source/COPYING" "$source_tar" "$wallet_bin/miners/"
printf '%s\n' "The miner, original source archive and license are in $wallet_bin/miners."
