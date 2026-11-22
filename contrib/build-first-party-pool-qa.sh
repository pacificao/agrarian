#!/usr/bin/env bash
# Build the loopback-only first-party worker without bringing the legacy
# miner, wallet, private data, or graphics drivers into the build.
set -euo pipefail
if [[ $# == 0 ]]; then
    printf 'Usage: %s NEW_OUTPUT_DIRECTORY [DEPENDENCY_INCLUDE_ROOT ...]\n' "$0" >&2
    exit 2
fi
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
dest=$1
shift
extra=()
for dir in "$@"; do
    include=$(realpath -e -- "$dir")
    [[ -d "$include" ]]
    extra+=("-I$include")
done
mkdir -- "$dest" # refuse to update or overwrite an existing build
dest=$(realpath -e -- "$dest")

qt=$(pkg-config --cflags --libs Qt6Core Qt6Network)
read -r -a qt_flags <<< "$qt"
objects=()
for hash in blake bmw groestl jh keccak skein; do
    object="$dest/$hash.o"
    "${CC:-cc}" -O2 -c "$root/src/crypto/$hash.c" -o "$object"
    objects+=("$object")
done
"${CXX:-c++}" -std=c++17 -O2 -I"$root/src" -c "$root/src/crypto/sha256.cpp" -o "$dest/sha256.o"
objects+=("$dest/sha256.o")
"${CXX:-c++}" -std=c++17 -O2 -I"$root/src" "${extra[@]}" \
    "$root/src/qt/agrarian_pool_worker.cpp" "${objects[@]}" "${qt_flags[@]}" \
    -o "$dest/agrarian-pool-worker"
"${CXX:-c++}" -std=c++17 -O2 -I"$root/src" "${extra[@]}" \
    "$root/test/functional/poolwork_test.cpp" "${objects[@]}" -o "$dest/poolwork-test"
"${CXX:-c++}" -std=c++17 -O2 -I"$root/src" \
    "$root/test/functional/poolframes_test.cpp" "${qt_flags[@]}" -o "$dest/poolframes-test"
"$dest/poolwork-test"
"$dest/poolframes-test"
python3 -B "$root/test/functional/first_party_pool_guard.py" "$dest/agrarian-pool-worker"
printf 'First-party staged worker validated: %s\n' "$dest/agrarian-pool-worker"
