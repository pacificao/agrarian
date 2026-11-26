#!/usr/bin/env python3
"""Exercise a real OpenCL GPU against an isolated Stratum fixture."""

import re
import sys
import tempfile
from pathlib import Path

from first_party_pool_guard import authorize, expect_quiet, listener, read, send_work, worker


def exercise(binary, kernel, job, options, cool_sensor=None):
    with listener() as pool:
        pool.settimeout(45)  # The vendor compiler may be cold on a new container.
        with worker(binary, pool, str(kernel), "--temperature-limit", "75", *options,
                    mode="--regtest-opencl") as (process, conn, stream):
            authorize(conn, stream, "ab12cd34", ".agrarian-qt-gpu")
            send_work(conn, job)
            if cool_sensor:
                expect_quiet(conn, 1.3)
                cool_sensor.write_text("50000\n")
            conn.settimeout(20)
            share = read(stream)
            assert share["method"] == "mining.submit" and share["params"][1] == job, share
            process.terminate()
            stdout, stderr = process.communicate(timeout=5)
            assert b"GPU Quark self-test passed" in stdout and b"MINING hashing" in stdout, (stdout, stderr)
            return stdout


def main(binary, kernel):
    live = exercise(binary, kernel, "real-nvidia-sensor", [])
    assert re.search(rb"THERMAL (?:normal|throttling) [0-9]+\.[0-9] - direct", live), live
    print("NVIDIA temperature reading, GPU self-test, and local share submission passed.", flush=True)
    with tempfile.TemporaryDirectory(prefix="agr-gpu-thermal-") as directory:
        sensor = Path(directory) / "temperature"
        sensor.write_text("95000\n")
        options = ["--regtest-thermal-sensor", str(sensor)]
        guarded = exercise(binary, kernel, "guarded", options, cool_sensor=sensor)
        assert b"THERMAL cooling 95.0 - direct" in guarded and b"MINING paused" in guarded, guarded
        print("GPU Thermal Guard withheld shares on simulated overheat and resumed after cooling.", flush=True)
        sensor.write_text("95000\n")
        bypassed = exercise(binary, kernel, "guard-bypassed", [*options, "--disable-thermal-guard"])
        assert b"MINING thermal-guard-disabled" in bypassed and b"THERMAL " not in bypassed, bypassed
        print("GPU bypass submitted a local share without reading the simulated hot sensor.", flush=True)


if __name__ == "__main__":
    main(*(Path(arg).resolve(strict=True) for arg in sys.argv[1:]))
