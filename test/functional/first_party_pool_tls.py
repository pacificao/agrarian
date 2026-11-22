#!/usr/bin/env python3
"""Test the exact fixed-endpoint client in a disposable Linux container.

Container requirements: --network none --add-host alltheacronyms.com:127.0.0.1,
Python, OpenSSL, update-ca-certificates, and this test run as container root.
"""

import json
import socket
import ssl
import subprocess
import sys
import tempfile
from pathlib import Path


def openssl(*args):
    subprocess.run(["openssl", *map(str, args)], check=True, capture_output=True)


def certificate(work, name, key, ca, ca_key):
    request = work / f"{name}.csr"
    cert = work / f"{name}.crt"
    extensions = work / f"{name}.ext"
    extensions.write_text(f"subjectAltName=DNS:{name}\nextendedKeyUsage=serverAuth\n")
    openssl("req", "-new", "-newkey", "rsa:2048", "-nodes", "-keyout", key,
            "-subj", f"/CN={name}", "-out", request)
    openssl("x509", "-req", "-in", request, "-CA", ca, "-CAkey", ca_key,
            "-CAcreateserial", "-days", "1", "-extfile", extensions, "-out", cert)
    return cert


def run(binary, listener, context, expected):
    fixture = "G" + "R" * 33
    with subprocess.Popen([binary, "--pool-cpu", fixture],
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE) as worker:
        try:
            sock, _ = listener.accept()
            with sock:
                sock.settimeout(6)
                if context is None:
                    # A plaintext Stratum server cannot elicit subscription data.
                    assert sock.recv(1) == b"\x16", "Expected only a TLS handshake, not a Stratum request"
                    sock.sendall(b'{"id":1,"result":[[],"ab12cd34",4],"error":null}\n')
                else:
                    try:
                        client = context.wrap_socket(sock, server_side=True)
                    except ssl.SSLError:
                        if expected:
                            raise
                    else:
                        with client, client.makefile("rb") as stream:
                            assert expected, "Worker accepted a wrong or untrusted certificate"
                            request = json.loads(stream.readline(4096))
                            assert request["method"] == "mining.subscribe"
                            client.sendall(b'{"id":1,"result":[[],"ab12cd34",4],"error":null}\n')
                            request = json.loads(stream.readline(4096))
                            assert request["method"] == "mining.authorize"
                            assert request["params"] == [fixture + ".agrarian-qt-cpu", "x"]
            _, stderr = worker.communicate(timeout=12)
            if not expected:
                assert worker.returncode == 1 and b"verified TLS connection" in stderr, stderr
        finally:
            if worker.poll() is None:
                worker.kill()


def main(binary):
    hostname = "alltheacronyms.com"
    if not Path("/.dockerenv").exists():
        raise SystemExit("Refusing to update the CA store outside a disposable Docker container")
    active = {name for _, name in socket.if_nameindex()
              if int((Path("/sys/class/net") / name / "flags").read_text(), 16) & 1}
    if active != {"lo"} or socket.gethostbyname(hostname) != "127.0.0.1":
        raise SystemExit("This test requires a private loopback-only container and hostname mapping")
    with tempfile.TemporaryDirectory() as raw, socket.socket() as listener:
        work = Path(raw)
        ca = work / "test-ca.crt"
        ca_key, server_key = work / "ca.key", work / "server.key"
        openssl("req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "1", "-keyout", ca_key,
                "-subj", "/CN=Agrarian disposable test CA", "-out", ca)
        listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        listener.bind(("127.0.0.1", 3033))
        listener.listen(1)
        listener.settimeout(10)
        right = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        right.load_cert_chain(certificate(work, hostname, server_key, ca, ca_key), server_key)
        run(binary, listener, None, False)
        run(binary, listener, right, False)
        subprocess.run(["install", "-m", "644", ca, "/usr/local/share/ca-certificates/agrarian-disposable.crt"],
                       check=True)
        subprocess.run(["update-ca-certificates"], check=True, capture_output=True)
        wrong = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        wrong.load_cert_chain(certificate(work, "wrong.example", server_key, ca, ca_key), server_key)
        run(binary, listener, wrong, False)
        run(binary, listener, right, True)
    print("Fixed pool TLS: plaintext, untrusted CA and wrong hostname rejected; verified connection authorized.")


if __name__ == "__main__":
    main(str(Path(sys.argv[1]).resolve(strict=True)))
