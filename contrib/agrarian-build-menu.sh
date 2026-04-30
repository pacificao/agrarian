#!/usr/bin/env bash
set -euo pipefail

REPO_URL="${REPO_URL:-https://github.com/pacificao/agrarian.git}"
WORKDIR="${WORKDIR:-$HOME/agrarian}"
HOST_WIN64="${HOST_WIN64:-x86_64-w64-mingw32}"

MENU_CHOICE="${AGRARIAN_MENU_CHOICE:-}"
ROOT=""

detect_script_branch() {
  local script_dir
  script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

  git -C "$script_dir" rev-parse --abbrev-ref HEAD 2>/dev/null || true
}

if [[ -z "${BRANCH:-}" ]]; then
  BRANCH="$(detect_script_branch)"
fi
BRANCH="${BRANCH:-main}"

detect_build_jobs() {
  if command -v nproc >/dev/null 2>&1; then
    nproc
  elif command -v getconf >/dev/null 2>&1; then
    getconf _NPROCESSORS_ONLN
  else
    echo 1
  fi
}

JOBS="${JOBS:-$(detect_build_jobs)}"

if [[ "${EUID:-$(id -u)}" -eq 0 && "${ALLOW_ROOT_BUILD_MENU:-0}" != "1" ]]; then
  cat >&2 <<EOF
Do not run this script with sudo.

Run it as your normal local user:
  ./contrib/agrarian-build-menu.sh

The script will ask for sudo only when it needs to install Ubuntu packages or
set the MinGW POSIX compiler alternatives. Repository checkout, compilation,
daemon config, and user systemd service setup run as the local user.
EOF
  exit 1
fi

has_cmd() {
  command -v "$1" >/dev/null 2>&1
}

progress() {
  local percent="$1"
  local message="$2"
  local width=30
  local filled=$((percent * width / 100))
  local empty=$((width - filled))
  local bar
  bar="$(printf '%*s' "$filled" '' | tr ' ' '#')"
  bar+="$(printf '%*s' "$empty" '' | tr ' ' '-')"
  printf '\n[%s] %3d%%  %s\n' "$bar" "$percent" "$message"
}

fail() {
  echo
  echo "ERROR: $*" >&2
  exit 1
}

run_step() {
  local percent="$1"
  local message="$2"
  shift 2
  progress "$percent" "$message"
  "$@"
}

prompt() {
  local question="$1"
  local default="${2:-}"
  local answer
  read -r -p "$question${default:+ [$default]}: " answer
  echo "${answer:-$default}"
}

confirm() {
  local question="$1"
  local answer
  read -r -p "$question [y/N]: " answer
  [[ "$answer" == "y" || "$answer" == "Y" || "$answer" == "yes" || "$answer" == "YES" ]]
}

select_target() {
  if [[ -n "$MENU_CHOICE" ]]; then
    return 0
  fi

  if has_cmd whiptail; then
    MENU_CHOICE="$(whiptail --title "Agrarian Build Menu" --menu "Select a build target" 17 76 8 \
      "linux-daemon" "Compile Linux daemon and CLI tools" \
      "linux-qt" "Compile Linux Qt GUI wallet" \
      "windows-daemon" "Cross-compile Windows daemon and CLI tools" \
      "windows-qt" "Cross-compile Windows Qt GUI wallet" \
      3>&1 1>&2 2>&3)" || exit 0
  else
    echo "Agrarian Build Menu"
    echo "1) Compile Linux daemon and CLI tools"
    echo "2) Compile Linux Qt GUI wallet"
    echo "3) Cross-compile Windows daemon and CLI tools"
    echo "4) Cross-compile Windows Qt GUI wallet"
    local choice
    read -r -p "Selection [1-4]: " choice
    case "$choice" in
      1) MENU_CHOICE="linux-daemon" ;;
      2) MENU_CHOICE="linux-qt" ;;
      3) MENU_CHOICE="windows-daemon" ;;
      4) MENU_CHOICE="windows-qt" ;;
      *) fail "Invalid selection: $choice" ;;
    esac
  fi
}

sudo_cmd() {
  if [[ "${EUID:-$(id -u)}" -eq 0 ]]; then
    "$@"
  elif has_cmd sudo; then
    sudo "$@"
  else
    fail "sudo is required to install missing packages. Install sudo or run as root."
  fi
}

apt_source_text() {
  grep -RhsE '^(deb |Types:|URIs:|Suites:|Components:)' \
    /etc/apt/sources.list /etc/apt/sources.list.d/*.list /etc/apt/sources.list.d/*.sources \
    2>/dev/null || true
}

ubuntu_sources_need_repair() {
  [[ -r /etc/os-release ]] || return 1
  # shellcheck disable=SC1091
  . /etc/os-release
  [[ "${ID:-}" == "ubuntu" && -n "${VERSION_CODENAME:-}" ]] || return 1

  local sources
  sources="$(apt_source_text)"
  [[ "$sources" == *"$VERSION_CODENAME-updates"* && "$sources" == *"$VERSION_CODENAME-security"* ]] && return 1
  return 0
}

repair_ubuntu_sources() {
  [[ -r /etc/os-release ]] || return 0
  # shellcheck disable=SC1091
  . /etc/os-release
  [[ "${ID:-}" == "ubuntu" && -n "${VERSION_CODENAME:-}" ]] || return 0

  local arch uri security_uri source_file
  arch="$(dpkg --print-architecture 2>/dev/null || true)"
  case "$arch" in
    arm64|armhf|ppc64el|riscv64|s390x)
      uri="http://ports.ubuntu.com/ubuntu-ports"
      security_uri="http://ports.ubuntu.com/ubuntu-ports"
      ;;
    *)
      uri="http://archive.ubuntu.com/ubuntu"
      security_uri="http://security.ubuntu.com/ubuntu"
      ;;
  esac
  source_file="/etc/apt/sources.list.d/agrarian-ubuntu.sources"

  cat >&2 <<EOF

This Ubuntu image appears to be missing standard apt suites such as
$VERSION_CODENAME-updates or $VERSION_CODENAME-security.

These suites are required for normal build packages like build-essential,
dpkg-dev, and bzip2 to resolve correctly.
EOF

  if ! confirm "Add standard Ubuntu main/universe apt sources for this architecture?"; then
    return 0
  fi

  local temp_file
  temp_file="$(mktemp)"
  cat > "$temp_file" <<EOF
Types: deb
URIs: $uri
Suites: $VERSION_CODENAME $VERSION_CODENAME-updates
Components: main universe
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg

Types: deb
URIs: $security_uri
Suites: $VERSION_CODENAME-security
Components: main universe
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
EOF

  sudo_cmd install -m 0644 "$temp_file" "$source_file"
  rm -f "$temp_file"
  echo "Added $source_file"
}

install_packages() {
  has_cmd apt-get || fail "This installer currently supports Ubuntu/Debian apt-get hosts."

  local packages=(
    ca-certificates git build-essential pkg-config autoconf automake libtool
    bsdmainutils cmake ninja-build python3 curl make tar patch bzip2 xz-utils
  )

  case "$MENU_CHOICE" in
    linux-qt)
      packages+=(
        xvfb
        libfontconfig1-dev libfreetype6-dev
        libx11-xcb-dev libxcb1-dev libxcb-cursor-dev libxcb-image0-dev
        libxcb-icccm4-dev libxcb-keysyms1-dev libxcb-randr0-dev
        libxcb-render0-dev libxcb-render-util0-dev libxcb-shape0-dev
        libxcb-shm0-dev libxcb-sync-dev libxcb-util-dev libxcb-xfixes0-dev
        libxcb-xinerama0-dev libxcb-xkb-dev libxi-dev libxrender-dev
        libxkbcommon-dev libxkbcommon-x11-dev
      )
      ;;
    windows-daemon|windows-qt)
      packages+=(mingw-w64 g++-mingw-w64-x86-64 g++-mingw-w64-x86-64-posix)
      ;;
  esac

  export DEBIAN_FRONTEND=noninteractive
  if ubuntu_sources_need_repair; then
    repair_ubuntu_sources
  fi
  sudo_cmd apt-get update
  if ! sudo_cmd apt-get install -y "${packages[@]}"; then
    cat >&2 <<EOF

Package installation failed.

On a minimal Ubuntu image, this is often an apt source configuration problem.
Check that the standard Ubuntu repositories are enabled, then rerun this script:

  sudo apt-cache policy bzip2 build-essential dpkg-dev
  sudo apt-get update

EOF
    exit 1
  fi
}

install_bootstrap_packages() {
  has_cmd apt-get || return 0
  has_cmd git && return 0

  export DEBIAN_FRONTEND=noninteractive
  if ubuntu_sources_need_repair; then
    repair_ubuntu_sources
  fi
  sudo_cmd apt-get update
  sudo_cmd apt-get install -y ca-certificates git
}

ensure_repo() {
  WORKDIR="$(prompt "Repository directory" "$WORKDIR")"
  JOBS="$(prompt "Parallel build jobs" "$JOBS")"

  if [[ -d "$WORKDIR/.git" ]]; then
    ROOT="$WORKDIR"
    run_step 25 "Fetching existing Agrarian checkout" git -C "$ROOT" fetch origin
    run_step 30 "Checking out $BRANCH" git -C "$ROOT" checkout "$BRANCH"
    run_step 35 "Fast-forwarding $BRANCH" git -C "$ROOT" pull --ff-only origin "$BRANCH"
  else
    mkdir -p "$(dirname "$WORKDIR")"
    run_step 35 "Cloning Agrarian into $WORKDIR" git clone --branch "$BRANCH" "$REPO_URL" "$WORKDIR"
    ROOT="$WORKDIR"
  fi

  cd "$ROOT"
}

reexec_from_checkout() {
  [[ "${AGRARIAN_REEXECED:-0}" == "1" ]] && return 0

  local repo_script
  repo_script="$ROOT/contrib/agrarian-build-menu.sh"
  [[ -x "$repo_script" ]] || return 0

  repo_script="$(cd "$(dirname "$repo_script")" && pwd -P)/$(basename "$repo_script")"

  echo
  echo "Restarting with the updated build menu from the checkout..."
  exec env \
    AGRARIAN_REEXECED=1 \
    AGRARIAN_MENU_CHOICE="$MENU_CHOICE" \
    BRANCH="$BRANCH" \
    WORKDIR="$WORKDIR" \
    JOBS="$JOBS" \
    HOST_WIN64="$HOST_WIN64" \
    "$repo_script"
}

ensure_posix_mingw() {
  local gcc_path="/usr/bin/$HOST_WIN64-gcc-posix"
  local gxx_path="/usr/bin/$HOST_WIN64-g++-posix"

  if [[ -x "$gcc_path" && -x "$gxx_path" ]]; then
    sudo_cmd update-alternatives --set "$HOST_WIN64-gcc" "$gcc_path" >/dev/null || true
    sudo_cmd update-alternatives --set "$HOST_WIN64-g++" "$gxx_path" >/dev/null || true
  fi

  has_cmd "$HOST_WIN64-g++" || fail "Missing $HOST_WIN64-g++ after package install."
  "$HOST_WIN64-g++" --version | head -n 1 | grep -qi posix || fail "$HOST_WIN64-g++ is not using the POSIX thread model."
}

build_windows_daemon() {
  ensure_posix_mingw

  run_step 45 "Building Windows daemon depends" make -C depends HOST="$HOST_WIN64" NO_QT=1 -j"$JOBS"

  if [[ ! -f configure ]]; then
    run_step 60 "Generating configure script" ./autogen.sh
  fi

  run_step 72 "Configuring Windows daemon build" env CONFIG_SITE="$ROOT/depends/$HOST_WIN64/share/config.site" ./configure \
    --prefix=/ \
    --without-gui \
    --disable-maintainer-mode \
    --disable-tests \
    --disable-bench \
    --disable-zmq \
    --with-miniupnpc=no

  run_step 90 "Compiling Windows daemon and CLI tools" make -j"$JOBS"
}

build_selected() {
  case "$MENU_CHOICE" in
    linux-daemon)
      run_step 45 "Compiling Linux daemon and CLI tools" env JOBS="$JOBS" ./contrib/build-linux.sh
      ;;
    linux-qt)
      run_step 45 "Compiling Linux Qt GUI wallet" env JOBS="$JOBS" ./contrib/build-linux-wallet.sh
      ;;
    windows-daemon)
      build_windows_daemon
      ;;
    windows-qt)
      run_step 45 "Compiling Windows Qt GUI wallet" env JOBS="$JOBS" ./contrib/build-win64-wallet.sh
      ;;
    *)
      fail "Unknown build choice: $MENU_CHOICE"
      ;;
  esac
}

install_user_daemon_service() {
  local bindir="$HOME/.local/bin"
  local confdir="$HOME/.agrarian"
  local systemd_dir="$HOME/.config/systemd/user"
  local service_file="$systemd_dir/agrariand.service"
  local conf_file="$confdir/agrarian.conf"
  local rpcpass

  [[ -x "$ROOT/src/agrariand" ]] || fail "Linux daemon binary not found at src/agrariand."
  [[ -x "$ROOT/src/agrarian-cli" ]] || fail "Linux CLI binary not found at src/agrarian-cli."
  has_cmd systemctl || fail "systemctl is required for user service setup."

  mkdir -p "$bindir" "$confdir" "$systemd_dir"
  install -m 0755 "$ROOT/src/agrariand" "$bindir/agrariand"
  install -m 0755 "$ROOT/src/agrarian-cli" "$bindir/agrarian-cli"

  if [[ ! -f "$conf_file" ]]; then
    if has_cmd openssl; then
      rpcpass="$(openssl rand -hex 32)"
    else
      rpcpass="$(date +%s)-$RANDOM-$RANDOM"
    fi
    cat > "$conf_file" <<EOF
server=1
daemon=0
listen=1
dnsseed=1
rpcuser=agrarianrpc
rpcpassword=$rpcpass
rpcbind=127.0.0.1
rpcallowip=127.0.0.1
EOF
    chmod 0600 "$conf_file"
  fi

  cat > "$service_file" <<EOF
[Unit]
Description=Agrarian daemon
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
ExecStart=$bindir/agrariand -conf=$conf_file -datadir=$confdir
ExecStop=$bindir/agrarian-cli -conf=$conf_file -datadir=$confdir stop
Restart=on-failure
RestartSec=10
TimeoutStopSec=120

[Install]
WantedBy=default.target
EOF

  systemctl --user daemon-reload
  systemctl --user enable --now agrariand.service

  if has_cmd loginctl; then
    loginctl enable-linger "$USER" >/dev/null 2>&1 || true
  fi

  echo
  echo "Agrarian daemon service is installed for the current user."
  echo "Config: $conf_file"
  echo "Service: $service_file"
  echo "Status: systemctl --user status agrariand"
}

show_completion() {
  progress 100 "Build finished"
  echo

  case "$MENU_CHOICE" in
    linux-daemon)
      echo "Linux daemon binaries:"
      echo "  $ROOT/src/agrariand"
      echo "  $ROOT/src/agrarian-cli"
      echo "  $ROOT/src/agrarian-tx"
      if confirm "Start agrariand now and enable automatic start at boot for the current user?"; then
        install_user_daemon_service
      else
        echo "Start manually with: $ROOT/src/agrariand -daemon"
      fi
      ;;
    linux-qt)
      echo "Linux wallet binaries:"
      echo "  $ROOT/src/qt/agrarian-qt"
      echo "  $ROOT/src/agrariand"
      echo "  $ROOT/src/agrarian-cli"
      if confirm "Start agrariand now and enable automatic start at boot for the current user?"; then
        install_user_daemon_service
      fi
      ;;
    windows-daemon)
      echo "Windows daemon binaries:"
      echo "  $ROOT/src/agrariand.exe"
      echo "  $ROOT/src/agrarian-cli.exe"
      echo "  $ROOT/src/agrarian-tx.exe"
      echo
      echo "Copy those .exe files to a folder on the Windows machine."
      echo "Start the daemon from PowerShell or Command Prompt:"
      echo "  agrariand.exe"
      echo "Then use:"
      echo "  agrarian-cli.exe help"
      ;;
    windows-qt)
      echo "Windows wallet binaries:"
      echo "  $ROOT/src/qt/agrarian-qt.exe"
      echo "  $ROOT/src/agrariand.exe"
      echo "  $ROOT/src/agrarian-cli.exe"
      echo "  $ROOT/src/agrarian-tx.exe"
      echo
      echo "Copy those .exe files to a folder on the Windows machine."
      echo "Launch the GUI wallet with:"
      echo "  agrarian-qt.exe"
      ;;
  esac
}

main() {
  select_target
  progress 5 "Selected target: $MENU_CHOICE"
  run_step 10 "Installing bootstrap Ubuntu packages" install_bootstrap_packages
  ensure_repo
  reexec_from_checkout
  run_step 40 "Installing required Ubuntu packages" install_packages
  build_selected
  show_completion
}

main "$@"
