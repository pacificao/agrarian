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

The preview does not download or install miners. It expects a separately
reviewed Quark-capable `cpuminer` or `cpuminer.exe` under `miners/` next to
`agrarian-qt`. The source release does not bundle that helper. For a
disposable Linux package, install the compiler and development libraries
(`autoconf automake libtool build-essential pkg-config libcurl4-openssl-dev
libssl-dev libjansson-dev zlib1g-dev` on Ubuntu), then run:

```sh
bash contrib/build-desktop-pool-miner.sh /path/to/staging/agrarian/bin
```

The script checks a pinned archive, builds locally, and keeps the original
source archive and license beside the helper. It never starts mining. Before a
one-click release, build it from reviewed, pinned source, include its license
and corresponding source with any distributed binary, and smoke-test the exact
Windows and Ubuntu packages. The [pool's current miner](https://github.com/tpruvot/cpuminer-multi)
is a starting point, not an approval of its existing upstream binaries.

Verify startup, disconnect/reconnect, accepted/rejected shares, clean shutdown
and payout credit on an isolated test pool. Use a disposable wallet, check it
owns the payout address, and verify that configured CPU thread counts and
resource usage match. Do not override operating system antivirus or execution
policy. A missing or blocked miner must fail with a visible explanation.

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
