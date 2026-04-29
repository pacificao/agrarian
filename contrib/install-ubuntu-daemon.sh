#!/usr/bin/env bash
set -euo pipefail

REPO_URL="${REPO_URL:-https://github.com/pacificao/agrarian.git}"
BRANCH="${BRANCH:-main}"
INSTALL_DIR="${INSTALL_DIR:-/opt/agrarian}"
AGRARIAN_USER="${AGRARIAN_USER:-agrarian}"
DATA_DIR="${DATA_DIR:-/var/lib/agrarian}"
CONF_DIR="${CONF_DIR:-/etc/agrarian}"
CONF_FILE="${CONF_FILE:-$CONF_DIR/agrarian.conf}"
SERVICE_FILE="${SERVICE_FILE:-/etc/systemd/system/agrariand.service}"
JOBS="${JOBS:-$(nproc)}"
RPCUSER="${RPCUSER:-agrarianrpc}"
RPCPASSWORD="${RPCPASSWORD:-$(openssl rand -hex 32)}"

if [[ "${EUID:-$(id -u)}" -ne 0 ]]; then
  echo "Run as root: sudo $0" >&2
  exit 1
fi

export DEBIAN_FRONTEND=noninteractive

apt-get update
apt-get install -y \
  git ca-certificates build-essential pkg-config autoconf automake libtool \
  bsdmainutils libboost-all-dev libevent-dev libgmp-dev libssl-dev \
  libdb5.3-dev libdb5.3++-dev

if ! id "$AGRARIAN_USER" >/dev/null 2>&1; then
  useradd --system --home "$DATA_DIR" --shell /usr/sbin/nologin "$AGRARIAN_USER"
fi

mkdir -p "$(dirname "$INSTALL_DIR")" "$DATA_DIR" "$CONF_DIR"

if [[ -d "$INSTALL_DIR/.git" ]]; then
  git -C "$INSTALL_DIR" fetch origin
  git -C "$INSTALL_DIR" checkout "$BRANCH"
  git -C "$INSTALL_DIR" pull --ff-only origin "$BRANCH"
else
  rm -rf "$INSTALL_DIR"
  git clone --branch "$BRANCH" "$REPO_URL" "$INSTALL_DIR"
fi

cd "$INSTALL_DIR"
chmod +x ./autogen.sh ./contrib/build-linux.sh
JOBS="$JOBS" ./contrib/build-linux.sh

install -m 0755 src/agrariand /usr/local/bin/agrariand
install -m 0755 src/agrarian-cli /usr/local/bin/agrarian-cli

chown -R "$AGRARIAN_USER:$AGRARIAN_USER" "$DATA_DIR" "$CONF_DIR"

if [[ ! -f "$CONF_FILE" ]]; then
  install -m 0640 -o "$AGRARIAN_USER" -g "$AGRARIAN_USER" /dev/null "$CONF_FILE"
  cat > "$CONF_FILE" <<EOF
server=1
daemon=0
listen=1
dnsseed=1
txindex=1
rpcuser=$RPCUSER
rpcpassword=$RPCPASSWORD
rpcbind=127.0.0.1
rpcallowip=127.0.0.1
EOF
  chown "$AGRARIAN_USER:$AGRARIAN_USER" "$CONF_FILE"
  chmod 0640 "$CONF_FILE"
fi

cat > "$SERVICE_FILE" <<EOF
[Unit]
Description=Agrarian daemon
After=network-online.target
Wants=network-online.target

[Service]
User=$AGRARIAN_USER
Group=$AGRARIAN_USER
Type=simple
ExecStart=/usr/local/bin/agrariand -conf=$CONF_FILE -datadir=$DATA_DIR
ExecStop=/usr/local/bin/agrarian-cli -conf=$CONF_FILE -datadir=$DATA_DIR stop
Restart=on-failure
RestartSec=10
TimeoutStopSec=120
PrivateTmp=true
ProtectSystem=full
NoNewPrivileges=true

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable --now agrariand

echo "Agrarian daemon installed and started."
echo "Branch: $BRANCH"
echo "Install: $INSTALL_DIR"
echo "Config: $CONF_FILE"
echo "Data: $DATA_DIR"
