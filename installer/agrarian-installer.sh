#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

HOST="x86_64-pc-linux-gnu"
ACTION="all"
WALLET="1"
JOBS="$(nproc)"
DO_UPDATE=0
RESET_DEPENDS=0
ASSUME_YES=0
USER_LOG_PATH=""
TEST_MODE="${AGRARIAN_INSTALLER_TEST_MODE:-0}"

usage() {
  cat <<'USAGE'
Usage: installer/agrarian-installer.sh [options]

Ubuntu-only installer for Agrarian build steps.

Options:
  --host <triplet>          Build host triplet (default: x86_64-pc-linux-gnu)
  --action <depends|daemon|qt|all>
                            Action to run (default: all)
  --wallet <0|1>            Enable wallet-related dependencies/build flags (default: 1)
  --jobs <n>                Parallel build jobs (default: nproc)
  --update                  Run git pull --rebase before build
  --reset-depends           Remove depends/work, depends/built, depends/<host>
  --yes                     Assume yes for prompts
  --log <path>              Log output path
  -h, --help                Show this help
USAGE
}

fail() {
  echo "ERROR: $*" >&2
  exit 1
}

check_ubuntu() {
  if [[ ! -r /etc/os-release ]]; then
    fail "Cannot verify OS (missing /etc/os-release); Ubuntu is required"
  fi

  # shellcheck disable=SC1091
  source /etc/os-release
  if [[ "${ID:-}" != "ubuntu" && "${ID_LIKE:-}" != *"ubuntu"* ]]; then
    fail "This installer only supports Ubuntu (detected ID='${ID:-unknown}')"
  fi
}

confirm() {
  local prompt="$1"
  if [[ "$ASSUME_YES" == "1" ]]; then
    return 0
  fi

  read -r -p "${prompt} [y/N]: " response
  case "$response" in
    y|Y|yes|YES)
      return 0
      ;;
    *)
      echo "Aborted."
      exit 1
      ;;
  esac
}

run_cmd() {
  echo "+ $*"
  if [[ "$TEST_MODE" == "1" ]]; then
    echo "[test-mode] skipped"
    return 0
  fi
  "$@"
}

ensure_configure() {
  if [[ ! -x "${REPO_ROOT}/configure" ]]; then
    run_cmd "${REPO_ROOT}/autogen.sh"
  fi
}

reset_depends_paths() {
  local host_prefix="${REPO_ROOT}/depends/${HOST}"
  local depends_work="${REPO_ROOT}/depends/work"
  local depends_built="${REPO_ROOT}/depends/built"

  echo "Resetting depends paths:"
  echo "  - ${depends_work}"
  echo "  - ${depends_built}"
  echo "  - ${host_prefix}"

  rm -rf "${depends_work}" "${depends_built}" "${host_prefix}"
}

build_depends() {
  local args=("HOST=${HOST}" "-j${JOBS}")
  if [[ "$WALLET" == "0" ]]; then
    args+=("USE_WALLET=0")
  fi
  run_cmd make -C "${REPO_ROOT}/depends" "${args[@]}"
}

ensure_depends_prereqs() {
  local prefix="${REPO_ROOT}/depends/${HOST}"
  local missing=()

  [[ -f "${prefix}/share/config.site" ]] || missing+=("${prefix}/share/config.site")
  [[ -f "${prefix}/include/boost/thread.hpp" ]] || missing+=("${prefix}/include/boost/thread.hpp")

  if ! compgen -G "${prefix}/lib/libboost_thread*.a" > /dev/null; then
    missing+=("${prefix}/lib/libboost_thread*.a")
  fi

  if ! compgen -G "${prefix}/lib/libboost_system*.a" > /dev/null; then
    missing+=("${prefix}/lib/libboost_system*.a")
  fi

  if [[ "$WALLET" == "1" && ! -f "${prefix}/include/db_cxx.h" ]]; then
    missing+=("${prefix}/include/db_cxx.h")
  fi

  if (( ${#missing[@]} > 0 )); then
    echo "ERROR: depends prefix is missing required files:" >&2
    printf '  - %s\n' "${missing[@]}" >&2
    echo "Fix: make -C ${REPO_ROOT}/depends HOST=${HOST} USE_WALLET=${WALLET} -j${JOBS}" >&2
    exit 1
  fi
}

configure_project() {
  local mode="$1"
  local config_site="${REPO_ROOT}/depends/${HOST}/share/config.site"
  local cfg_args=("--prefix=${REPO_ROOT}/depends/${HOST}")

  [[ "$WALLET" == "0" ]] && cfg_args+=("--disable-wallet")
  [[ "$mode" == "daemon" ]] && cfg_args+=("--without-gui")

  ensure_configure
  ensure_depends_prereqs
  run_cmd env CONFIG_SITE="${config_site}" "${REPO_ROOT}/configure" "${cfg_args[@]}"
}

build_daemon() {
  configure_project daemon
  run_cmd make -C "${REPO_ROOT}/src" "-j${JOBS}" agrariand
}

build_qt() {
  configure_project qt
  run_cmd make -C "${REPO_ROOT}/src/qt" "-j${JOBS}" agrarian-qt
}

build_all() {
  configure_project all
  run_cmd make -C "${REPO_ROOT}" "-j${JOBS}"
}

needs_depends() {
  [[ ! -f "${REPO_ROOT}/depends/${HOST}/share/config.site" ]]
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --host)
      [[ $# -ge 2 ]] || fail "Missing value for --host"
      HOST="$2"
      shift 2
      ;;
    --action)
      [[ $# -ge 2 ]] || fail "Missing value for --action"
      ACTION="$2"
      shift 2
      ;;
    --wallet)
      [[ $# -ge 2 ]] || fail "Missing value for --wallet"
      WALLET="$2"
      shift 2
      ;;
    --jobs)
      [[ $# -ge 2 ]] || fail "Missing value for --jobs"
      JOBS="$2"
      shift 2
      ;;
    --update)
      DO_UPDATE=1
      shift
      ;;
    --reset-depends)
      RESET_DEPENDS=1
      shift
      ;;
    --yes)
      ASSUME_YES=1
      shift
      ;;
    --log)
      [[ $# -ge 2 ]] || fail "Missing value for --log"
      USER_LOG_PATH="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      fail "Unknown option: $1"
      ;;
  esac
done

case "$ACTION" in
  depends|daemon|qt|all) ;;
  *) fail "Invalid --action: ${ACTION}" ;;
esac

case "$WALLET" in
  0|1) ;;
  *) fail "Invalid --wallet value: ${WALLET} (expected 0 or 1)" ;;
esac

[[ "$JOBS" =~ ^[0-9]+$ ]] || fail "--jobs must be a positive integer"
(( JOBS > 0 )) || fail "--jobs must be greater than 0"

if [[ -n "$USER_LOG_PATH" ]]; then
  LOG_PATH="$USER_LOG_PATH"
else
  mkdir -p "${REPO_ROOT}/installer/logs"
  LOG_PATH="${REPO_ROOT}/installer/logs/agrarian-installer-$(date +%Y%m%d-%H%M%S).log"
fi
mkdir -p "$(dirname "${LOG_PATH}")"

# Start logging after CLI parse so --log applies to all run output.
exec > >(tee -a "${LOG_PATH}") 2>&1

echo "Agrarian installer"
echo "  repo: ${REPO_ROOT}"
echo "  host: ${HOST}"
echo "  action: ${ACTION}"
echo "  wallet: ${WALLET}"
echo "  jobs: ${JOBS}"
echo "  update: ${DO_UPDATE}"
echo "  reset_depends: ${RESET_DEPENDS}"
echo "  test_mode: ${TEST_MODE}"
echo "  log: ${LOG_PATH}"

check_ubuntu

confirm "Proceed with selected installer action?"

if [[ "$DO_UPDATE" == "1" ]]; then
  run_cmd git -C "${REPO_ROOT}" pull --rebase
fi

if [[ "$RESET_DEPENDS" == "1" ]]; then
  confirm "Delete depends/work, depends/built, and depends/${HOST}?"
  reset_depends_paths
fi

case "$ACTION" in
  depends)
    build_depends
    ;;
  daemon)
    if needs_depends; then
      build_depends
    fi
    build_daemon
    ;;
  qt)
    if needs_depends; then
      build_depends
    fi
    build_qt
    ;;
  all)
    build_depends
    build_all
    ;;
esac

echo "Completed successfully."
