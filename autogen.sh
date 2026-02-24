#!/bin/sh
# Copyright (c) 2013-2016 The Bitcoin Core developers
# Copyright (c) 2026 Agrarian Developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

# Fail fast on errors and undefined vars. (Note: 'pipefail' is not POSIX sh.)
set -eu

srcdir="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)"
cd "$srcdir"

# Prefer glibtoolize on macOS if LIBTOOLIZE isn't already set.
if [ -z "${LIBTOOLIZE:-}" ]; then
    GLIBTOOLIZE="$(command -v glibtoolize 2>/dev/null || true)"
    if [ -n "$GLIBTOOLIZE" ]; then
        LIBTOOLIZE="$GLIBTOOLIZE"
        export LIBTOOLIZE
    fi
fi

if ! command -v autoreconf >/dev/null 2>&1; then
    echo "configuration failed: please install autoconf first" >&2
    exit 1
fi

autoreconf --install --force --warnings=all
