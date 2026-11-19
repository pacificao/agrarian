#!/usr/bin/env bash
set -euo pipefail

if [[ $# != 2 || ! -d "$2" || -L "$2" ]]; then
  echo "Usage: $0 COMMIT EMPTY_OUTPUT_DIRECTORY" >&2
  exit 2
fi
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
target="$(realpath -e -- "$2")"
first_entry="$(find "$target" -mindepth 1 -maxdepth 1 -print -quit)"
[[ -z "$first_entry" ]] || { echo "Output directory must be empty" >&2; exit 2; }
revision="$(git -C "$repo" rev-parse --verify "$1^{commit}")"

# These files came from an earlier host build and must not enter release builds.
git -C "$repo" archive "$revision" -- . \
  ':(exclude)depends/built' ':(exclude)depends/sources' \
  ':(exclude)depends/x86_64-pc-linux-gnu' \
  ':(exclude,glob)**/.deps/**' ':(exclude,glob)**/.libs/**' \
  ':(exclude,glob)**/autom4te.cache/**' \
  ':(exclude,glob)**/*.o' ':(exclude,glob)**/*.lo' ':(exclude,glob)**/*.la' \
  ':(exclude,glob)**/config.log' ':(exclude,glob)**/config.status' \
  ':(exclude,glob)**/libtool' ':(exclude,glob)**/stamp-h1' \
  ':(exclude)Makefile' ':(exclude)src/Makefile' ':(exclude)doc/man/Makefile' \
  ':(exclude)src/univalue/Makefile' ':(exclude)src/config/agrarian-config.h' \
  | tar -xf - -C "$target"
printf '%s\n' "$revision"
