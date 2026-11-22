#!/usr/bin/env bash
# Build the optional desktop pool helper from pinned sources.
set -euo pipefail

if [[ $# != 1 || ! -d "$1" ]]; then
    printf 'Usage: %s WALLET_BIN_DIRECTORY\nSet HOST=x86_64-w64-mingw32 and MINGW_DEPENDS for Win64.\n' "$0" >&2
    exit 2
fi
wallet_bin=$(realpath -e -- "$1")
[[ "$wallet_bin" != / ]]
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

variant=cpuminer
sources=("$source_tar")
configure_args=(--disable-assembly --with-crypto=/usr --with-curl=/usr)
miner_cppflags=""
miner_ldflags=""
miner_libs=""
if [[ ${HOST:-} == x86_64-w64-mingw32 ]]; then
    command -v "$HOST-gcc"
    mdep=$(realpath -e -- "${MINGW_DEPENDS:?Set MINGW_DEPENDS to the Win64 depends prefix}")
    [[ -f "$mdep/lib/libcrypto.a" && -f "$mdep/lib/libz.a" ]]
    [[ -n ${MINGW_SOURCES:-} && -d "$MINGW_SOURCES" ]]
    root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
    for dep in openssl zlib; do
        version=$(awk -F= '$1 == "$(package)_version" {print $2}' "$root/depends/packages/$dep.mk")
        sha=$(awk -F= '$1 == "$(package)_sha256_hash" {print $2}' "$root/depends/packages/$dep.mk")
        [[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ && "$sha" =~ ^[[:xdigit:]]{64}$ ]]
        archive="${MINGW_SOURCES:?}/$dep-$version.tar.gz"
        printf '%s  %s\n' "$sha" "$archive" | sha256sum -c --strict
        sources+=("$archive")
    done

    curl_version=8.23.0
    curl_sha=8dadd7d2536a314f982ad05a651b713f426bf7d02cc2c76cffb9796f2218e6c5
    curl_tar="$work/curl-$curl_version.tar.xz"
    curl --fail --silent --show-error --location --connect-timeout 10 --max-time 90 \
        --retry 3 --output "$curl_tar" "https://curl.se/download/curl-$curl_version.tar.xz"
    printf '%s  %s\n' "$curl_sha" "$curl_tar" | sha256sum -c --strict
    mkdir "$work/curl-source"
    tar --extract --xz --no-same-owner --strip-components=1 --file "$curl_tar" --directory "$work/curl-source"
    (
        cd "$work/curl-source"
        CC="$HOST-gcc" ./configure --host="$HOST" --prefix="$work/curl" \
            --disable-shared --enable-static --disable-docs --disable-manual \
            --without-ssl --without-zlib --without-brotli --without-zstd --without-libpsl \
            --without-libidn2 --without-nghttp2 --without-librtmp --without-libssh2 \
            --disable-ldap --disable-ldaps --disable-ftp --disable-file --disable-dict \
            --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb \
            --disable-smtp --disable-gopher --disable-mqtt --disable-rtsp
        make -C lib -j"$jobs"
        make -C lib install
        make -C include install
    )
    install -m 0644 "$work/curl-source/COPYING" "$work/COPYING-curl"
    sources+=("$curl_tar" "$work/COPYING-curl")
    configure_args=(--host="$HOST" --disable-assembly --with-crypto="$mdep" --with-curl="$work/curl")
    variant=cpuminer.exe
    miner_cppflags=-DCURL_STATICLIB
    miner_ldflags=-static
    miner_libs="-lwinpthread -lws2_32 -lcrypt32 -lgdi32 -liphlpapi -lbcrypt"
    # Use the toolchain's winpthreads, not headers and objects bundled
    # in the upstream archive for the older pthreads-win32 toolchain.
    sed -i '/CFLAGS="-Icompat\/pthreads /d; /PTHREAD_LDFLAGS="-Lcompat\/pthreads\//d' "$work/source/configure.ac"
    # MinGW declares socklen_t in ws2tcpip.h, not winsock2.h.
    sed -i '/^# include <winsock2\.h>$/a # include <ws2tcpip.h>' "$work/source/api.c"
elif [[ -n ${HOST:-} ]]; then
    printf 'Unsupported miner target: %s\n' "$HOST" >&2
    exit 2
fi
(
    cd "$work/source"
    bash ./autogen.sh
    CFLAGS="-O2 -fcommon -std=gnu11" CPPFLAGS="$miner_cppflags" LDFLAGS="$miner_ldflags" \
        LIBS="$miner_libs" ./configure "${configure_args[@]}"
    make -j"$jobs"
    if [[ "$variant" == cpuminer ]]; then
        ./cpuminer --version
    else
        "$HOST-objdump" -f "$variant" | grep -F 'file format pei-x86-64'
        "$HOST-objdump" -p "$variant" | awk '/DLL Name:/ {print tolower($3)}' |
            while IFS= read -r dll; do
                case "$dll" in
                    bcrypt.dll|crypt32.dll|gdi32.dll|iphlpapi.dll|kernel32.dll|msvcrt.dll|user32.dll|ws2_32.dll|advapi32.dll)
                        ;;
                    *) printf 'Unexpected Windows miner DLL: %s\n' "$dll" >&2; exit 1 ;;
                esac
            done
    fi
)

mkdir "$wallet_bin/miners"
install -m 0755 "$work/source/$variant" "$wallet_bin/miners/"
install -m 0644 "$work/source/COPYING" "${sources[@]}" "$wallet_bin/miners/"
printf '%s\n' "The miner, original source archive and license are in $wallet_bin/miners."
