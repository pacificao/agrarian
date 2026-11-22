# First-party Agrarian pool worker (2.1.3 staging)

The wallet will keep one mining dialog. Run the bundled worker in a separate,
unprivileged process to keep crashes and mining logic outside the wallet.
Process separation alone is not a sandbox: where supported, restrict the
worker's access to wallet files and the wallet process at the OS level. It
receives a public payout address, an explicit CPU/GPU choice, a bounded
workload and a stop signal. It does not use the wallet RPC, read wallet files,
or start without the user's action.

This branch contains testable building blocks, not a complete miner:

- `src/poolwork.h` builds Agrarian's 80-byte Quark work header from the pool's
  previous hash, coinbase and Merkle branches. It scans bounded CPU batches
  with cancellation and provides a full 256-bit verifier for candidates from
  either backend. The negotiated pool share target remains distinct from the
  network target in the block header.
- `src/qt/poolframes.h` classifies only bounded Stratum frames and ignores
  server redirects and other unsolicited commands. It does not establish a
  network connection. The later transport must bound unread data, correlate
  replies, reject malformed job fields, and distinguish current and stale work.
- The mining log uses explicit near-white text on a dark background. Its
  Qt regression check exercises the actual widget palette.

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

## Before a release

1. Implement the first-party connection and worker lifecycle, with no redirects,
   no remote control listener, no external configuration or auto-downloads, and
   a fixed production pool destination. Use loopback only for regtest fixtures.
2. Connect the tested OpenCL kernel to the worker and verify candidates on the
   CPU before submission. Keep CPU mode available without silently starting it
   when the GPU cannot run. Qualify real NVIDIA and AMD hardware.
3. Test CPU and GPU shares, block and payout, reconnect and stale work, shutdown,
   and missing drivers. Verify the exact installed Windows and Linux
   packages, independently rebuild and compare provenance, and scan with
   current endpoint protection enabled. A scan is evidence, not a guarantee.
4. The Windows candidate must no longer include the legacy `cpuminer.exe`;
   test a normal upgrade from 2.1.2 without altering wallet data. Keep the old
   sample quarantined and submit only the official release copy for Microsoft's
   adjudication. Do not recommend antivirus exclusions.
