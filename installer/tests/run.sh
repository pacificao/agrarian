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

echo "[test 1] help output works"
bash "${WORK_REPO}/installer/agrarian-installer.sh" --help > "${LOG_DIR}/help.out"
rg -q -- "--action <depends|daemon|qt|all>" "${LOG_DIR}/help.out"

echo "[test 2] depends build for x86_64-pc-linux-gnu succeeds"
DEPENDS_LOG="${LOG_DIR}/depends.log"
PATH="${FAKE_BIN}:${PATH}" AGRARIAN_FAKE_MAKE_LOG="${FAKE_MAKE_LOG}" bash "${WORK_REPO}/installer/agrarian-installer.sh" \
  --action depends \
  --host x86_64-pc-linux-gnu \
  --jobs 2 \
  --yes \
  --log "${DEPENDS_LOG}"
assert_file_nonempty "${DEPENDS_LOG}"
rg -q -- "-C ${WORK_REPO}/depends HOST=x86_64-pc-linux-gnu -j2" "${FAKE_MAKE_LOG}"

echo "[test 3] can run twice with --reset-depends"
for i in 1 2; do
  RESET_LOG="${LOG_DIR}/reset-${i}.log"
  AGRARIAN_INSTALLER_TEST_MODE=1 bash "${WORK_REPO}/installer/agrarian-installer.sh" \
    --action depends \
    --host x86_64-pc-linux-gnu \
    --jobs 2 \
    --reset-depends \
    --yes \
    --log "${RESET_LOG}"
  assert_file_nonempty "${RESET_LOG}"
done

echo "[test 4] all actions produce logs and return 0"
for action in depends daemon qt all; do
  ACTION_LOG="${LOG_DIR}/${action}.log"
  AGRARIAN_INSTALLER_TEST_MODE=1 bash "${WORK_REPO}/installer/agrarian-installer.sh" \
    --action "${action}" \
    --host x86_64-pc-linux-gnu \
    --jobs 2 \
    --yes \
    --log "${ACTION_LOG}"
  assert_file_nonempty "${ACTION_LOG}"
done

echo "All installer tests passed."
