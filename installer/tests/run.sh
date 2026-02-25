#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
TMP_ROOT="$(mktemp -d)"
WORK_REPO="${TMP_ROOT}/repo"
LOG_DIR="${TMP_ROOT}/logs"
FAKE_BIN="${TMP_ROOT}/fake-bin"
FAKE_MAKE_LOG="${TMP_ROOT}/fake-make.log"

cleanup() {
  rm -rf "${TMP_ROOT}"
}
trap cleanup EXIT

mkdir -p "${LOG_DIR}" "${FAKE_BIN}"

echo "[setup] creating temporary repo copy"
rsync -a \
  --exclude '.git' \
  --exclude 'installer/logs' \
  --exclude 'installer/tests/tmp' \
  "${REPO_ROOT}/" "${WORK_REPO}/"

cat > "${FAKE_BIN}/make" <<'SH'
#!/usr/bin/env bash
set -Eeuo pipefail

echo "$*" >> "${AGRARIAN_FAKE_MAKE_LOG}"
exit 0
SH
chmod +x "${FAKE_BIN}/make"

for tool in \
  x86_64-w64-mingw32-g++ \
  i686-w64-mingw32-g++ \
  aarch64-linux-gnu-gcc \
  aarch64-linux-gnu-g++ \
  arm-linux-gnueabihf-gcc \
  arm-linux-gnueabihf-g++; do
  cat > "${FAKE_BIN}/${tool}" <<'SH'
#!/usr/bin/env bash
exit 0
SH
  chmod +x "${FAKE_BIN}/${tool}"
done

assert_file_nonempty() {
  local path="$1"
  local i
  for i in $(seq 1 20); do
    if [[ -s "$path" ]]; then
      return 0
    fi
    sleep 0.1
  done
  echo "Expected non-empty file: $path" >&2
  exit 1
}

seed_fake_depends_prefix() {
  local host="$1"
  local prefix="${WORK_REPO}/depends/${host}"
  mkdir -p "${prefix}/share" "${prefix}/include/boost" "${prefix}/lib" "${prefix}/lib/pkgconfig"
  : > "${prefix}/share/config.site"
  : > "${prefix}/include/boost/thread.hpp"
  : > "${prefix}/include/db_cxx.h"
  : > "${prefix}/lib/libboost_thread.a"
  : > "${prefix}/lib/libboost_system.a"
  : > "${prefix}/lib/pkgconfig/Qt5Core.pc"
  : > "${prefix}/lib/pkgconfig/Qt5Gui.pc"
  : > "${prefix}/lib/pkgconfig/Qt5Network.pc"
  : > "${prefix}/lib/pkgconfig/Qt5Widgets.pc"
}

echo "[test 1] help output works"
bash "${WORK_REPO}/installer/agrarian-installer.sh" --help > "${LOG_DIR}/help.out"
rg -q -- "--action <depends|daemon|qt|all>" "${LOG_DIR}/help.out"
rg -q -- "--qt-target <native|win64|win32|armhf|aarch64|all>" "${LOG_DIR}/help.out"
rg -q -- "--host <triplet>" "${LOG_DIR}/help.out"
rg -q -- "--wallet <0|1>" "${LOG_DIR}/help.out"
rg -q -- "--jobs <n>" "${LOG_DIR}/help.out"
rg -q -- "--reset-depends" "${LOG_DIR}/help.out"
rg -q -- "--update" "${LOG_DIR}/help.out"
rg -q -- "--yes" "${LOG_DIR}/help.out"
rg -q -- "--dry-run" "${LOG_DIR}/help.out"

echo "[test 2] depends build for x86_64-pc-linux-gnu succeeds"
DEPENDS_LOG="${LOG_DIR}/depends.log"
PATH="${FAKE_BIN}:${PATH}" AGRARIAN_FAKE_MAKE_LOG="${FAKE_MAKE_LOG}" bash "${WORK_REPO}/installer/agrarian-installer.sh" \
  --action depends \
  --host x86_64-pc-linux-gnu \
  --jobs 2 \
  --yes \
  --log "${DEPENDS_LOG}"
assert_file_nonempty "${DEPENDS_LOG}"
rg -q -- "-C ${WORK_REPO}/depends HOST=x86_64-pc-linux-gnu USE_WALLET=1 -j2" "${FAKE_MAKE_LOG}"

echo "[test 3] wallet=0 maps to USE_WALLET=0 for depends"
DEPENDS_W0_LOG="${LOG_DIR}/depends-wallet0.log"
PATH="${FAKE_BIN}:${PATH}" AGRARIAN_FAKE_MAKE_LOG="${FAKE_MAKE_LOG}" bash "${WORK_REPO}/installer/agrarian-installer.sh" \
  --action depends \
  --host x86_64-pc-linux-gnu \
  --wallet 0 \
  --jobs 2 \
  --yes \
  --log "${DEPENDS_W0_LOG}"
assert_file_nonempty "${DEPENDS_W0_LOG}"
rg -q -- "-C ${WORK_REPO}/depends HOST=x86_64-pc-linux-gnu USE_WALLET=0 -j2" "${FAKE_MAKE_LOG}"

echo "[test 4] reset-depends only removes allowed paths"
mkdir -p "${WORK_REPO}/depends/work" "${WORK_REPO}/depends/built" "${WORK_REPO}/depends/x86_64-pc-linux-gnu" "${WORK_REPO}/user-runtime-data"
: > "${WORK_REPO}/user-runtime-data/keep-me"
RESET_LOG="${LOG_DIR}/reset.log"
PATH="${FAKE_BIN}:${PATH}" AGRARIAN_INSTALLER_TEST_MODE=1 bash "${WORK_REPO}/installer/agrarian-installer.sh" \
  --action depends \
  --host x86_64-pc-linux-gnu \
  --jobs 2 \
  --reset-depends \
  --yes \
  --log "${RESET_LOG}"
assert_file_nonempty "${RESET_LOG}"
[[ ! -d "${WORK_REPO}/depends/work" ]]
[[ ! -d "${WORK_REPO}/depends/built" ]]
[[ ! -d "${WORK_REPO}/depends/x86_64-pc-linux-gnu" ]]
[[ -f "${WORK_REPO}/user-runtime-data/keep-me" ]]

echo "[test 5] all actions produce logs and return 0"
seed_fake_depends_prefix "x86_64-pc-linux-gnu"
for action in depends daemon qt all; do
  ACTION_LOG="${LOG_DIR}/${action}.log"
  PATH="${FAKE_BIN}:${PATH}" AGRARIAN_INSTALLER_TEST_MODE=1 bash "${WORK_REPO}/installer/agrarian-installer.sh" \
    --action "${action}" \
    --host x86_64-pc-linux-gnu \
    --jobs 2 \
    --yes \
    --log "${ACTION_LOG}"
  assert_file_nonempty "${ACTION_LOG}"
done

echo "[test 6] dry-run qt win64 prints expected commands"
QT_DRY_LOG="${LOG_DIR}/qt-win64-dry.log"
PATH="${FAKE_BIN}:${PATH}" bash "${WORK_REPO}/installer/agrarian-installer.sh" \
  --action qt \
  --qt-target win64 \
  --jobs 2 \
  --yes \
  --dry-run \
  --log "${QT_DRY_LOG}"
assert_file_nonempty "${QT_DRY_LOG}"
rg -q -- "\\+ make -C ${WORK_REPO}/depends HOST=x86_64-w64-mingw32 USE_WALLET=1 -j2 qt" "${QT_DRY_LOG}"
rg -q -- "CONFIG_SITE=${WORK_REPO}/depends/x86_64-w64-mingw32/share/config.site" "${QT_DRY_LOG}"
rg -q -- "--build=x86_64-w64-mingw32" "${QT_DRY_LOG}"
rg -q -- "--host=x86_64-w64-mingw32" "${QT_DRY_LOG}"
rg -q -- "\\+ make -C ${WORK_REPO}/src/qt -j2 agrarian-qt" "${QT_DRY_LOG}"

echo "All installer tests passed."
