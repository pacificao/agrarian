#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

HOST="x86_64-pc-linux-gnu"
ACTION="all"
QT_TARGET="native"
WALLET="1"
JOBS="$(nproc)"
DO_UPDATE=0
RESET_DEPENDS=0
ASSUME_YES=0
USER_LOG_PATH=""
TEST_MODE="${AGRARIAN_INSTALLER_TEST_MODE:-0}"
DRY_RUN="0"
SUPPORTED_HOSTS=()

usage() {
  cat <<'USAGE'
Usage: installer/agrarian-installer.sh [options]

Ubuntu-only installer for Agrarian build steps.

Options:
  --host <triplet>          Build host triplet (default: x86_64-pc-linux-gnu)
  --action <depends|daemon|qt|all>
                            Action to run (default: all)
  --qt-target <native|win64|win32|armhf|aarch64|all>
                            Qt wallet target (default: native)
  --wallet <0|1>            Enable wallet-related dependencies/build flags (default: 1)
  --jobs <n>                Parallel build jobs (default: nproc)
  --update                  Run git pull --rebase before build
  --reset-depends           Remove depends/work, depends/built, depends/<host>
  --yes                     Assume yes for prompts
  --log <path>              Log output path
  --dry-run                 Print commands without executing them
  -h, --help                Show this help
USAGE
}

fail() {
  echo "ERROR: $*" >&2
  exit 1
}

join_by() {
  local IFS="$1"
  shift
  echo "$*"
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
  if is_dry_run; then
    echo "[dry-run] skipped"
    return 0
  fi
  "$@"
}

detect_supported_hosts() {
  local host_dir="${REPO_ROOT}/depends/hosts"
  local name
  local skip
  local generic
  local known_generics=("default" "linux" "mingw32" "darwin")

  [[ -d "${host_dir}" ]] || fail "Missing depends hosts directory: ${host_dir}"

  SUPPORTED_HOSTS=()
  for path in "${host_dir}"/*.mk; do
    [[ -f "${path}" ]] || continue
    name="$(basename "${path}" .mk)"
    skip=0
    for generic in "${known_generics[@]}"; do
      if [[ "${name}" == "${generic}" ]]; then
        skip=1
        break
      fi
    done
    (( skip == 0 )) && SUPPORTED_HOSTS+=("${name}")
  done

  ((${#SUPPORTED_HOSTS[@]} > 0)) || fail "No supported host definitions found in ${host_dir}"
}

ensure_supported_host() {
  local host="$1"
  local supported
  supported="$(join_by ", " "${SUPPORTED_HOSTS[@]}")"
  if [[ ! " ${SUPPORTED_HOSTS[*]} " =~ [[:space:]]${host}[[:space:]] ]]; then
    fail "Unsupported host '${host}'. Supported hosts: ${supported}"
  fi
}

add_missing_toolchain() {
  local tool="$1"
  local package="$2"

  if ! command -v "${tool}" >/dev/null 2>&1; then
    MISSING_TOOLS+=("${tool}")
    MISSING_PACKAGES+=("${package}")
  fi
}

check_toolchains_for_host() {
  local host="$1"
  case "${host}" in
    x86_64-w64-mingw32)
      add_missing_toolchain "x86_64-w64-mingw32-g++" "g++-mingw-w64-x86-64"
      ;;
    i686-w64-mingw32)
      add_missing_toolchain "i686-w64-mingw32-g++" "g++-mingw-w64-i686"
      ;;
    arm-linux-gnueabihf)
      add_missing_toolchain "arm-linux-gnueabihf-g++" "g++-arm-linux-gnueabihf"
      add_missing_toolchain "arm-linux-gnueabihf-ar" "binutils-arm-linux-gnueabihf"
      ;;
    aarch64-unknown-linux-gnu)
      add_missing_toolchain "aarch64-linux-gnu-g++" "g++-aarch64-linux-gnu"
      add_missing_toolchain "aarch64-linux-gnu-ar" "binutils-aarch64-linux-gnu"
      ;;
    *)
      ;;
  esac
}

ensure_toolchains_for_hosts() {
  local hosts=("$@")
  local unique_packages=()
  local host
  local pkg

  MISSING_TOOLS=()
  MISSING_PACKAGES=()

  for host in "${hosts[@]}"; do
    check_toolchains_for_host "${host}"
  done

  if ((${#MISSING_TOOLS[@]} == 0)); then
    return 0
  fi

  for pkg in "${MISSING_PACKAGES[@]}"; do
    if [[ ! " ${unique_packages[*]} " =~ [[:space:]]${pkg}[[:space:]] ]]; then
      unique_packages+=("${pkg}")
    fi
  done

  echo "ERROR: Missing required cross toolchain binaries:" >&2
  printf '  - %s\n' "${MISSING_TOOLS[@]}" >&2
  echo "Install with:" >&2
  echo "  sudo apt-get update && sudo apt-get install -y $(join_by " " "${unique_packages[@]}")" >&2
  exit 1
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
  local host="$1"
  local args=("HOST=${host}" "USE_WALLET=${WALLET}" "-j${JOBS}")
  run_cmd make -C "${REPO_ROOT}/depends" "${args[@]}"
}

build_depends_qt() {
  local host="$1"
  local args=("HOST=${host}" "USE_WALLET=${WALLET}" "-j${JOBS}" "qt")
  run_cmd make -C "${REPO_ROOT}/depends" "${args[@]}"
}

ensure_depends_prereqs() {
  local host="$1"
  local prefix="${REPO_ROOT}/depends/${host}"
  local missing=()

  if is_dry_run; then
    echo "[dry-run] skipping depends prefix checks for ${host}"
    return 0
  fi

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
    echo "Fix: make -C ${REPO_ROOT}/depends HOST=${host} USE_WALLET=${WALLET} -j${JOBS}" >&2
    exit 1
  fi
}

ensure_qt_pkgconfig_prereqs() {
  local host="$1"
  local prefix="${REPO_ROOT}/depends/${host}"
  local missing=()
  local module
  local found_path

  if is_dry_run; then
    echo "[dry-run] skipping Qt pkg-config checks for ${host}"
    return 0
  fi

  for module in Qt5Core Qt5Gui Qt5Network Qt5Widgets; do
    found_path=""
    if [[ -f "${prefix}/lib/pkgconfig/${module}.pc" ]]; then
      found_path="${prefix}/lib/pkgconfig/${module}.pc"
    elif [[ -f "${prefix}/share/pkgconfig/${module}.pc" ]]; then
      found_path="${prefix}/share/pkgconfig/${module}.pc"
    fi
    [[ -n "${found_path}" ]] || missing+=("${module}.pc in ${prefix}/(lib|share)/pkgconfig")
  done

  if (( ${#missing[@]} > 0 )); then
    echo "ERROR: Qt pkg-config files are missing from depends prefix:" >&2
    printf '  - %s\n' "${missing[@]}" >&2
    echo "Fix: make -C ${REPO_ROOT}/depends HOST=${host} USE_WALLET=${WALLET} -j${JOBS} qt" >&2
    exit 1
  fi
}

configure_project() {
  local mode="$1"
  local host="$2"
  local config_site="${REPO_ROOT}/depends/${host}/share/config.site"
  local cfg_args=(
    "--build=${host}"
    "--host=${host}"
    "--prefix=${REPO_ROOT}/depends/${host}"
    "--with-boost=${REPO_ROOT}/depends/${host}"
  )

  [[ "$WALLET" == "0" ]] && cfg_args+=("--disable-wallet")
  [[ "$mode" == "daemon" ]] && cfg_args+=("--without-gui")

  ensure_configure
  ensure_depends_prereqs "${host}"
  run_cmd env CONFIG_SITE="${config_site}" "${REPO_ROOT}/configure" "${cfg_args[@]}"
}

build_daemon() {
  configure_project daemon "${HOST}"
  run_cmd make -C "${REPO_ROOT}/src" "-j${JOBS}" agrariand
}

qt_target_host() {
  local target="$1"
  case "${target}" in
    native)
      echo "${HOST}"
      ;;
    win64)
      echo "x86_64-w64-mingw32"
      ;;
    win32)
      echo "i686-w64-mingw32"
      ;;
    armhf)
      echo "arm-linux-gnueabihf"
      ;;
    aarch64)
      echo "aarch64-unknown-linux-gnu"
      ;;
    *)
      return 1
      ;;
  esac
}

qt_target_output() {
  local host="$1"
  if [[ "${host}" == *mingw32 ]]; then
    echo "${REPO_ROOT}/src/qt/agrarian-qt.exe"
  else
    echo "${REPO_ROOT}/src/qt/agrarian-qt"
  fi
}

qt_target_list() {
  local target="$1"
  case "${target}" in
    all)
      echo "native win64 win32 armhf aarch64"
      ;;
    *)
      echo "${target}"
      ;;
  esac
}

build_qt_for_host() {
  local target="$1"
  local host="$2"
  local prefix="${REPO_ROOT}/depends/${host}"
  local output_path

  output_path="$(qt_target_output "${host}")"

  echo "Qt wallet build target:"
  echo "  target: ${target}"
  echo "  host: ${host}"
  echo "  depends prefix: ${prefix}"
  echo "  output: ${output_path}"

  build_depends_qt "${host}"
  ensure_qt_pkgconfig_prereqs "${host}"
  configure_project qt "${host}"
  run_cmd make -C "${REPO_ROOT}/src/qt" "-j${JOBS}" agrarian-qt
}

build_qt() {
  local target
  local host
  local qt_targets
  local unique_hosts=()
  local entry

  qt_targets="$(qt_target_list "${QT_TARGET}")"
  for target in ${qt_targets}; do
    host="$(qt_target_host "${target}")"
    if [[ -z "${host}" ]]; then
      fail "Invalid --qt-target value: ${target}"
    fi
    for entry in "${unique_hosts[@]}"; do
      if [[ "${entry}" == "${host}" ]]; then
        host=""
        break
      fi
    done
    [[ -n "${host}" ]] && unique_hosts+=("${host}")
  done

  ensure_toolchains_for_hosts "${unique_hosts[@]}"

  for target in ${qt_targets}; do
    host="$(qt_target_host "${target}")"
    build_qt_for_host "${target}" "${host}"
  done
}

build_all() {
  configure_project all "${HOST}"
  run_cmd make -C "${REPO_ROOT}" "-j${JOBS}"
}

needs_depends() {
  local host="$1"
  [[ ! -f "${REPO_ROOT}/depends/${host}/share/config.site" ]]
}

is_dry_run() {
  [[ "${TEST_MODE}" == "1" || "${DRY_RUN}" == "1" ]]
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
    --qt-target)
      [[ $# -ge 2 ]] || fail "Missing value for --qt-target"
      QT_TARGET="$2"
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
    --dry-run)
      DRY_RUN=1
      TEST_MODE=1
      shift
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

case "$QT_TARGET" in
  native|win64|win32|armhf|aarch64|all) ;;
  *) fail "Invalid --qt-target: ${QT_TARGET}" ;;
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
echo "  qt_target: ${QT_TARGET}"
echo "  wallet: ${WALLET}"
echo "  jobs: ${JOBS}"
echo "  update: ${DO_UPDATE}"
echo "  reset_depends: ${RESET_DEPENDS}"
echo "  dry_run: $(is_dry_run && echo 1 || echo 0)"
echo "  log: ${LOG_PATH}"

check_ubuntu
detect_supported_hosts

if [[ "${ACTION}" != "qt" && "${QT_TARGET}" != "native" ]]; then
  fail "--qt-target is only supported with --action qt"
fi

if [[ "${ACTION}" == "qt" ]]; then
  qt_targets="$(qt_target_list "${QT_TARGET}")"
  for target in ${qt_targets}; do
    host="$(qt_target_host "${target}")"
    [[ -n "${host}" ]] || fail "Invalid --qt-target value: ${target}"
    ensure_supported_host "${host}"
  done
else
  ensure_supported_host "${HOST}"
  ensure_toolchains_for_hosts "${HOST}"
fi

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
    build_depends "${HOST}"
    ;;
  daemon)
    if needs_depends "${HOST}"; then
      build_depends "${HOST}"
    fi
    build_daemon
    ;;
  qt)
    build_qt
    ;;
  all)
    build_depends "${HOST}"
    build_all
    ;;
esac

echo "Completed successfully."
