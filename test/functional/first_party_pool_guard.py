#!/usr/bin/env python3
"""Verify security boundaries of the first-party loopback-only Stratum worker."""

import json
import socket
import subprocess
import sys
import tempfile
from contextlib import contextmanager
from pathlib import Path

# Public fixture ID, not a real user's address or a wallet secret.
FIXTURE = "G" + "R" * 33


@contextmanager
def listener():
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        sock.listen()
        sock.settimeout(5)
        yield sock


@contextmanager
def worker(binary, pool):
    process = subprocess.Popen([str(binary), "--regtest-loopback", str(pool.getsockname()[1]), FIXTURE],
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        conn, _ = pool.accept()
        with conn, conn.makefile("rb") as stream:
            conn.settimeout(5)
            yield process, conn, stream
    finally:
        if process.poll() is None:
            process.terminate()
            process.wait(timeout=10)


def send(conn, message):
    conn.sendall(json.dumps(message).encode("ascii") + b"\n")


def read(stream):
    return json.loads(stream.readline(4096))


def main(binary):
    for mode in ("--mainnet", "--url", "--connect"):
        process = subprocess.run([str(binary), mode, "3032", FIXTURE], capture_output=True, timeout=5)
        assert process.returncode == 2 and b"Usage:" in process.stderr

    with tempfile.TemporaryDirectory(prefix="agrarian-pool-kernel-") as directory:
        corrupt = Path(directory) / "corrupt.cl"
        corrupt.write_bytes(bytes(186550))
        process = subprocess.run([str(binary), "--regtest-opencl", "3032", FIXTURE, str(corrupt)],
                                 capture_output=True, timeout=5)
        assert process.returncode == 1 and b"does not match the pinned source" in process.stderr

    with listener() as pool, listener() as redirected, worker(binary, pool) as (process, conn, stream):
        first = read(stream)
        assert first["id"] == 1 and first["method"] == "mining.subscribe"
        send(conn, {"id": None, "method": "client.reconnect",
                    "params": ["127.0.0.1", redirected.getsockname()[1], 0]})
        # Split a valid message across socket writes.
        conn.sendall(b'{"id":1,"result":[[],"ab12cd34",4],')
        conn.sendall(b'"error":null}\n')
        second = read(stream)
        assert second["id"] == 2 and second["method"] == "mining.authorize"
        assert second["params"] == [FIXTURE + ".agrarian-qt-cpu", "x"]
        send(conn, {"id": 2, "result": False, "error": None})
        stdout, stderr = process.communicate(timeout=5)
        assert process.returncode == 1 and b"rejected authorization" in stderr and not stdout
        redirected.settimeout(0.25)
        try:
            unexpected, _ = redirected.accept()
        except TimeoutError:
            pass
        else:
            unexpected.close()
            raise AssertionError("The worker honored a server redirect")

    with listener() as pool, worker(binary, pool) as (process, conn, stream):
        assert read(stream)["method"] == "mining.subscribe"
        conn.sendall(b"x" * (16 * 1024 + 1))
        stdout, stderr = process.communicate(timeout=5)
        assert process.returncode == 1 and b"Oversized incomplete pool frame" in stderr and not stdout

    print("First-party worker refused production arguments and an altered kernel, "
          "ignored a redirect, rejected unauthorized work, and bounded incomplete packets.")


if __name__ == "__main__":
    main(Path(sys.argv[1]).resolve(strict=True))
