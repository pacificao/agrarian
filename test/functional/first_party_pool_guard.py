#!/usr/bin/env python3
"""Verify fixed-destination and loopback Stratum security boundaries."""

import json
import socket
import subprocess
import sys
import tempfile
import time
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
def worker(binary, pool, *options):
    process = subprocess.Popen([str(binary), "--regtest-loopback", str(pool.getsockname()[1]), FIXTURE, *options],
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


def authorize(conn, stream, nonce):
    assert read(stream)["method"] == "mining.subscribe"
    send(conn, {"id": 1, "result": [[], nonce, 4], "error": None})
    request = read(stream)
    assert request["method"] == "mining.authorize"
    assert request["params"] == [FIXTURE + ".agrarian-qt-cpu", "x"]
    send(conn, {"id": 2, "result": True, "error": None})


def send_work(conn, job):
    # Make a share deterministic without contacting a live pool or wallet.
    send(conn, {"id": None, "method": "mining.set_difficulty", "params": [1e-12]})
    send(conn, {"id": None, "method": "mining.notify",
                "params": [job, "0" * 64, "01", "02", [], "00000001", "1d00ffff", "65432100", True]})


def expect_quiet(conn, duration):
    conn.settimeout(duration)
    try:
        packet = conn.recv(4096)
    except TimeoutError:
        pass
    else:
        raise AssertionError(f"Miner sent traffic while thermally paused: {packet!r}")
    finally:
        conn.settimeout(12)


def test_thermal_pause(binary):
    with tempfile.TemporaryDirectory(prefix="agr-thermal-fixture-") as directory:
        sensor = Path(directory) / "temperature"
        with listener() as pool, worker(binary, pool, "--temperature-limit", "75",
                                        "--regtest-thermal-sensor", str(sensor)) as (process, conn, stream):
            authorize(conn, stream, "ab12cd34")
            send_work(conn, "temperature-missing")
            expect_quiet(conn, 1.3)
            sensor.write_text("60000\n")
            share = read(stream)
            assert share["params"][1] == "temperature-missing"
            sensor.write_text("80000\n")
            time.sleep(1.25)
            send(conn, {"id": share["id"], "result": True, "error": None})
            send_work(conn, "after-cooldown")
            expect_quiet(conn, 1.25)
            sensor.write_text("65000\n")
            start = time.monotonic()
            share = read(stream)
            assert time.monotonic() - start >= 3.5
            assert share["params"][1] == "after-cooldown"
            process.terminate()
            stdout, _ = process.communicate(timeout=5)
            for status in (b"THERMAL unavailable", b"THERMAL normal", b"THERMAL cooling", b"THERMAL throttling"):
                assert status in stdout, stdout


def test_windows_thermal_proxy_policy(binary):
    with tempfile.TemporaryDirectory(prefix="agr-thermal-proxy-") as directory:
        sensor = Path(directory) / "temperature"
        sensor.write_text("65000\n")
        with listener() as pool, worker(binary, pool, "--temperature-limit", "75",
                                        "--regtest-thermal-sensor", str(sensor), "--regtest-thermal-proxy",
                                        "--allow-windows-thermal-proxy", "--allow-unmonitored-low-load") as (process, conn, stream):
            authorize(conn, stream, "ab12cd34")
            send_work(conn, "proxy-safe")
            expect_quiet(conn, 1.3)  # 65°C is below CPU target but exceeds proxy target
            sensor.write_text("50000\n")
            share = read(stream)
            assert share["params"][1] == "proxy-safe"
            sensor.unlink()
            time.sleep(1.2)
            send(conn, {"id": share["id"], "result": True, "error": None})
            send_work(conn, "proxy-lost")
            expect_quiet(conn, 1.3)  # even when unmonitored mode was also requested
            process.terminate()
            stdout, _ = process.communicate(timeout=5)
            assert b"THERMAL cooling 65.0 - proxy" in stdout and b"THERMAL unavailable" in stdout, stdout


def test_reconnect(binary):
    with listener() as pool, worker(binary, pool) as (process, first, first_stream):
        authorize(first, first_stream, "ab12cd34")
        send_work(first, "old-job")
        old_share = read(first_stream)
        assert old_share["method"] == "mining.submit" and old_share["params"][1] == "old-job"
        # Drop the socket before acknowledging the share.
        began = time.monotonic()
        first.shutdown(socket.SHUT_RDWR)

        second, _ = pool.accept()
        with second, second.makefile("rb") as stream:
            assert time.monotonic() - began >= 0.85, "Missing first reconnect backoff"
            second.settimeout(5)
            authorize(second, stream, "1234abcd")
            second.settimeout(0.25)
            try:
                unexpected = second.recv(4096)
            except TimeoutError:
                pass
            else:
                raise AssertionError(f"Unexpected traffic before receiving new work: {unexpected!r}")
            second.settimeout(5)
            send_work(second, "new-job")
            new_share = read(stream)
            assert new_share["method"] == "mining.submit" and new_share["params"][1] == "new-job"
            assert new_share["id"] == old_share["id"], "Request IDs should restart with the session"
            began = time.monotonic()
            second.shutdown(socket.SHUT_RDWR)

        third, _ = pool.accept()
        with third, third.makefile("rb") as stream:
            assert time.monotonic() - began >= 1.85, "A brief reconnect should not reset backoff"
            third.settimeout(5)
            assert read(stream)["method"] == "mining.subscribe"
            third.shutdown(socket.SHUT_RDWR)

        # The dialog's Stop action kills this process rather than waiting for backoff.
        pool.settimeout(0.5)
        try:
            unexpected, _ = pool.accept()
        except TimeoutError:
            pass
        else:
            unexpected.close()
            raise AssertionError("Missing backoff before third retry")
        assert process.poll() is None
        process.kill()
        stdout, _ = process.communicate(timeout=2)
        assert b"Pool reconnecting in 4." in stdout
        pool.settimeout(4.5)
        try:
            unexpected, _ = pool.accept()
        except TimeoutError:
            pass
        else:
            unexpected.close()
            raise AssertionError("Worker reconnected after Stop")


def main(binary):
    guarded = subprocess.run([str(binary), "--pool-cpu", FIXTURE, "--regtest-thermal-sensor", "ignored", "--temperature-limit", "75"],
                             capture_output=True, timeout=5)
    assert guarded.returncode == 2, "A fixture must never override mainnet temperatures"
    test_thermal_pause(binary)
    test_windows_thermal_proxy_policy(binary)
    for mode in ("--mainnet", "--url", "--connect"):
        process = subprocess.run([str(binary), mode, "3032", FIXTURE], capture_output=True, timeout=5)
        assert process.returncode == 2 and b"Usage:" in process.stderr
    process = subprocess.run([str(binary), "--pool-cpu", FIXTURE, "https://invalid.invalid"],
                             capture_output=True, timeout=5)
    assert process.returncode == 2 and b"Invalid CPU thread count" in process.stderr

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
        assert process.returncode == 1 and b"rejected authorization" in stderr and b"configured for 1 threads" in stdout
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
        assert process.returncode == 1 and b"Oversized incomplete pool frame" in stderr and b"configured for 1 threads" in stdout

    test_reconnect(binary)
    print("First-party worker refused arbitrary endpoints and an altered kernel, "
          "ignored a redirect, rejected unauthorized work, and bounded incomplete packets.")
    print("Temporary disconnects used increasing backoff and fresh Stratum state; Stop canceled retry.")


if __name__ == "__main__":
    main(Path(sys.argv[1]).resolve(strict=True))
