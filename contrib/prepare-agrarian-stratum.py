#!/usr/bin/env python3
"""Prepare, but do not install, a TLS listener for the production pool."""

import copy
import json
import os
import sys
from pathlib import Path

CONFIG = Path("/etc/miningcore/agrarian.config.json")
NGINX = Path("/etc/nginx/nginx.conf")
INCLUDE = Path("/etc/nginx/agrarian-stratum.conf")
HOOK = Path("/etc/letsencrypt/renewal-hooks/deploy/agrarian-stratum-nginx-reload")


def main(directory):
    os.umask(0o077)
    if not directory.is_dir() or any(directory.iterdir()):
        raise SystemExit("Provide an empty private staging directory")
    raw = CONFIG.read_bytes()
    nginx = NGINX.read_bytes()
    config = json.loads(raw)
    pools = [item for item in config["pools"] if item["id"] == "agrarian"]
    assert len(pools) == 1 and set(pools[0]["ports"]) == {"3032"}
    ports = pools[0]["ports"]
    assert ports["3032"]["listenAddress"] == "0.0.0.0"
    assert not ports["3032"].get("tls") and not ports["3032"].get("tcpProxyProtocol")
    assert not INCLUDE.exists() and not HOOK.exists() and b"agrarian-stratum" not in nginx
    ports["3034"] = {
        **copy.deepcopy(ports["3032"]),
        "name": ports["3032"]["name"] + " TLS backend",
        "listenAddress": "127.0.0.1",
        "tcpProxyProtocol": {"enable": True, "mandatory": True},
    }
    (directory / "miningcore.before").write_bytes(raw)
    (directory / "nginx.before").write_bytes(nginx)
    (directory / "miningcore.after").write_text(json.dumps(config, indent=2) + "\n")
    (directory / "nginx.after").write_bytes(nginx + b"\ninclude /etc/nginx/agrarian-stratum.conf;\n")
    (directory / "stream.conf").write_text("""stream {
    server {
        listen 3033 ssl;
        ssl_certificate /etc/letsencrypt/live/alltheacronyms.com/fullchain.pem;
        ssl_certificate_key /etc/letsencrypt/live/alltheacronyms.com/privkey.pem;
        ssl_protocols TLSv1.2 TLSv1.3;
        ssl_handshake_timeout 10s;
        proxy_connect_timeout 5s;
        proxy_timeout 5m;
        proxy_protocol on;
        proxy_pass 127.0.0.1:3034;
    }
}
""")
    (directory / "certbot-hook").write_text("""#!/bin/sh
set -eu
[ "${RENEWED_LINEAGE:-}" = /etc/letsencrypt/live/alltheacronyms.com ] || exit 0
/usr/sbin/nginx -t
/usr/bin/systemctl reload nginx
""")
    print(f"Prepared candidate and backups in {directory}; no services changed")


if __name__ == "__main__":
    main(Path(sys.argv[1]))
