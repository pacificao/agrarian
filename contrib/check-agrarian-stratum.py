#!/usr/bin/env python3
"""Subscribe (without authorizing or mining) to both production listeners."""

import json
import socket
import ssl
import sys

DOMAIN = "alltheacronyms.com"
SUBSCRIBE = b'{"id":1,"method":"mining.subscribe","params":["Agrarian TLS health check"]}\n'


def subscribe(host, port, secure):
    with socket.create_connection((host, port), timeout=6) as raw:
        context = ssl.create_default_context() if secure else None
        if context:
            context.minimum_version = ssl.TLSVersion.TLSv1_2
        conn = context.wrap_socket(raw, server_hostname=DOMAIN) if context else raw
        with conn, conn.makefile("rb") as lines:
            conn.sendall(SUBSCRIBE)
            for _ in range(12):
                line = lines.readline(16385)
                if not line or len(line) > 16384:
                    raise RuntimeError(f"Missing or oversized Stratum reply on {port}")
                reply = json.loads(line)
                if reply.get("id") == 1:
                    if reply.get("error") or not isinstance(reply.get("result"), list):
                        raise RuntimeError(f"Stratum subscription rejected on {port}")
                    print(f"{host}:{port} subscribe OK ({conn.version() if secure else 'legacy TCP'})")
                    return
            raise RuntimeError(f"No subscribe response on {port}")


if __name__ == "__main__":
    target = "127.0.0.1" if sys.argv[1:] == ["--local"] else DOMAIN
    subscribe(target, 3032, False)
    subscribe(target, 3033, True)
