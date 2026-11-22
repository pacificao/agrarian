# Desktop pool mining preview

This is a mainnet desktop feature, not a network-rule change. It leaves the
existing solo CPU mining and staking controls separate. Do not run a staging
build against the production pool just to test its GUI.

In a package containing a compatible CPU miner, choose **Mining → Mine on
All The Acronyms**, set the CPU thread count, and click **Start pool mining**.
One thread is the default. The desktop wallet chooses a receiving address
that it controls, and passes only that public address and a worker suffix to
the fixed pool. It does not pass wallet secrets. Closing the pool window or
quitting Agrarian Core stops the local miner. Accepted shares are not payouts.

## Packaging and release gates

The wallet does not download or install miners at runtime. Desktop candidates
build a Quark-capable `cpuminer` or `cpuminer.exe` under `miners/` next to
`agrarian-qt`. The source release does not bundle a compiled helper. For a
disposable Linux package, install the compiler and development libraries
(`autoconf automake libtool build-essential pkg-config libcurl4-openssl-dev
libssl-dev libjansson-dev zlib1g-dev` on Ubuntu), then run:

```sh
bash contrib/build-desktop-pool-miner.sh /path/to/staging/agrarian/bin
```

The script checks a pinned archive, builds locally, and keeps the original
source archive and license beside the helper. It never starts mining. The
Windows cross-build uses the existing wallet depends prefix for OpenSSL and
zlib and a pinned static [curl](https://curl.se/docs/install.html) build.
Smoke-test the exact Windows and Ubuntu packages before release; do not ship
precompiled miners from the [upstream project](https://github.com/tpruvot/cpuminer-multi).

Verify startup, disconnect/reconnect, accepted/rejected shares, clean shutdown
and payout credit on an isolated test pool. Use a disposable wallet, check it
owns the payout address, and verify that configured CPU thread counts and
resource usage match. Do not override operating system antivirus or execution
policy. A missing or blocked miner must fail with a visible explanation.

The integration test can drive the real packaged CPU helper:

```sh
python3 -B test/functional/miningcore_staging.py \
  --dotnet /path/to/dotnet --miningcore /path/to/Miningcore.dll \
  --pg-bin /path/to/postgresql/bin --schema /path/to/createdb.sql \
  --core-bin /path/to/staging/agrarian/bin \
  --cpuminer /path/to/staging/agrarian/bin/miners/cpuminer --split-difficulty
```

Run it as an unprivileged user in a network namespace with only loopback.
It generates disposable wallet data, verifies the worker's accepted share
and block, stops the worker, and checks that the separate wallet receives
test AGR. On Linux, add `--qt-wallet /path/to/staging/agrarian/bin/agrarian-qt`
and run under a private X11 display (for example, `xvfb-run -a`) with
`xdotool` installed to exercise the wallet's start, stop, and close controls.
This uses a regtest-only pool URL restricted to loopback; the production pool
address cannot be overridden. The Windows installer is tested separately.

## GPU target: 2.1.3

GPU mining is intentionally not included in the CPU preview. Qualify a
separate Quark backend for each advertised GPU family and operating system.
An initial NVIDIA CUDA candidate is [ccminer](https://github.com/tpruvot/ccminer);
check its build, licensing and current driver compatibility before choosing it.
Do not assume the same backend supports AMD. Report incompatible or missing
devices without starting a process. GPU device selection and intensity are
separate from CPU thread count.

Release only after clean-host install and real-device tests for discovery,
safe defaults, accepted shares and payout credit on staging, reconnect,
switching modes, and shutdown that leaves no mining process behind. GPU
mining must stay off by default and never start on login or on wallet launch.
