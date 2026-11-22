# First-party Agrarian pool worker (2.1.3 staging)

The wallet will keep one mining dialog. Run the bundled worker in a separate,
unprivileged process to keep crashes and mining logic outside the wallet.
Process separation alone is not a sandbox: where supported, restrict the
worker's access to wallet files and the wallet process at the OS level. It
receives a public payout address, an explicit CPU/GPU choice, a bounded
workload and a stop signal. It does not use the wallet RPC, read wallet files,
or start without the user's action.

This branch has a loopback-only pool client, not a release candidate:

- `src/poolwork.h` builds Agrarian's 80-byte Quark work header from the pool's
  previous hash, coinbase and Merkle branches. It scans bounded CPU batches
  with cancellation and provides a full 256-bit verifier for candidates from
  either backend. The negotiated pool share target remains distinct from the
  network target in the block header.
- `src/qt/poolframes.h` classifies only bounded Stratum frames and ignores
  server redirects and other unsolicited commands.
- `src/qt/agrarian_pool_worker.cpp` subscribes and authorizes against
  loopback, bounds unread data and job fields, correlates replies and
  submits Quark work. CPU work is still single-threaded in this prototype.
  It cannot accept any public pool URL and does not ship in release packages.
- `src/qt/poolopencl.h` loads OpenCL only in an explicitly selected mode,
  verifies the pinned kernel, qualifies devices against Core, then rehashes
  candidates on the CPU before passing them to the same Stratum client.
- The regtest-only wallet preview offers independent CPU and GPU switches.
  Both can run at once using one payout address and separate pool connections
  and worker names. The shared log uses explicit near-white text on a dark
  background and labels each line `[CPU]` or `[GPU]`.

## Local tests

From the repository root, after building Core's crypto archive:

```sh
c++ -std=c++17 -O2 -I src test/functional/poolwork_test.cpp \
  src/crypto/libbitcoin_crypto.a -o /tmp/agrarian-poolwork-test
/tmp/agrarian-poolwork-test
c++ -std=c++17 -O2 -I src test/functional/poolframes_test.cpp \
  $(pkg-config --cflags --libs Qt6Core) -o /tmp/agrarian-poolframes-test
/tmp/agrarian-poolframes-test
```

Alternatively, with C/C++ tools, Boost headers, Qt6 Core/Network headers,
OpenCL C headers and Python installed, use the standalone loopback build:

```sh
bash contrib/build-first-party-pool-qa.sh /tmp/agrarian-first-party-qa
```

The output directory must not exist. The script compiles the needed Core
hash sources and checks authorization, redirect handling, receive limits,
kernel pinning and refusal of public endpoints without any pool work.
For block/payout integration use `test/functional/miningcore_staging.py
--first-party-worker /path/to/agrarian-pool-worker` in a loopback-only
network namespace. `--software-opencl-kernel` exercises the same worker
with a QA-only software driver; it cannot certify real GPUs. Add
`--simultaneous` to require accepted shares from both worker names for the
same payout address, chain agreement, and a confirmed test payout.

## Before a release

1. Implement the production connection and integrate the worker lifecycle, with no redirects,
   no remote control listener, no external configuration or auto-downloads, and
   a fixed production pool destination. Use loopback only for regtest fixtures.
2. Qualify the integrated OpenCL path on real NVIDIA and AMD hardware. Keep
   CPU mode available without silently starting it when the GPU cannot run.
3. Test CPU and GPU shares, block and payout, reconnect and stale work, shutdown,
   and missing drivers. Verify the exact installed Windows and Linux
   packages, independently rebuild and compare provenance, and scan with
   current endpoint protection enabled. A scan is evidence, not a guarantee.
4. The Windows candidate must no longer include the legacy `cpuminer.exe`;
   test a normal upgrade from 2.1.2 without altering wallet data. Keep the old
   sample quarantined and submit only the official release copy for Microsoft's
   adjudication. Do not recommend antivirus exclusions.
