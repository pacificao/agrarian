#!/usr/bin/env bash
set -euo pipefail

REPO_URL="${REPO_URL:-https://github.com/pacificao/agrarian.git}"
BRANCH="${BRANCH:-2.0}"
WORKDIR="${WORKDIR:-$HOME/agrarian}"
JOBS="${JOBS:-1}"
HOST_WIN64="${HOST_WIN64:-x86_64-w64-mingw32}"

MENU_CHOICE=""
ROOT=""

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

install_packages() {
  has_cmd apt-get || fail "This installer currently supports Ubuntu/Debian apt-get hosts."

  local packages=(
    ca-certificates git build-essential pkg-config autoconf automake libtool
    bsdmainutils cmake ninja-build python3 curl make tar patch
  )

  case "$MENU_CHOICE" in
    linux-qt)
      packages+=(xvfb)
      ;;
    windows-daemon|windows-qt)
      packages+=(mingw-w64 g++-mingw-w64-x86-64 g++-mingw-w64-x86-64-posix)
      ;;
  esac

  export DEBIAN_FRONTEND=noninteractive
  sudo_cmd apt-get update
  sudo_cmd apt-get install -y "${packages[@]}"
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
  run_step 15 "Installing required Ubuntu packages" install_packages
  ensure_repo
  build_selected
  show_completion
}

main "$@"
