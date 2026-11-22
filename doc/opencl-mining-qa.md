# Agrarian 2.1.3 GPU engineering notes

This branch is not a release candidate. The public 2.1.2 wallet is unchanged.
The first gate is consensus parity: an OpenCL implementation must agree with
Agrarian Core on the full 32-byte Quark result, not just a hashrate readout.

## Offline parity test

The test reads a SHA-256-pinned [sgminer source revision][sgminer], expands
only the six named Quark includes, and makes portability adaptations:
declare the Groestl scratch arrays in kernel scope, use the compact tables,
pass the nonce base explicitly, and return all 32 digest bytes in Agrarian's
canonical order for every lane. It starts no miner, opens no pool connection,
and uses synthetic headers only. Software OpenCL is a separate QA mode; it
is not GPU qualification.

From the repo root after building the Core crypto archive:

```sh
c++ -std=c++17 -O2 -I src test/functional/opencl_quark_reference.cpp \
  src/crypto/libbitcoin_crypto.a -o /tmp/agrarian-quark-reference
curl --fail --location \
  https://codeload.github.com/nicehash/sgminer/tar.gz/002021095feb64065c902863cf717af7f94854bd \
  --output /tmp/agrarian-sgminer-source.tar.gz
python3 -B test/functional/opencl_quark_qa.py /tmp/agrarian-sgminer-source.tar.gz \
  --core-reference /tmp/agrarian-quark-reference --screen
```

Use `--export-reference /path/to/reference.bin` on the Core build host,
then `--reference-digests /path/to/reference.bin` instead of
`--core-reference` on a GPU test host of a different architecture.
`--screen` checks the smaller per-lane potential-share output against CPU
results, including target equality and a batch ending at the maximum nonce.
Any future worker must rehash flagged candidates and compare all 256 bits
before sending a share.

## Release gates

1. Qualify real NVIDIA and AMD cards against Core, including an older
   driver/card combination per vendor before advertising legacy coverage.
   Discover graphics devices through the installed OpenCL runtime rather
   than matching a hard-coded list of model names. ICD loaders still
   require a working vendor driver; a GPU's age cannot replace that requirement.
2. Connect the compute path to a bundled user-consented worker. Keep the
   proven pool protocol, pinned source and notices, no unauthenticated API,
   and no installer-time or wallet-startup work. Keep the endpoint fixed on
   mainnet. Limit kernel batches, independently verify share candidates
   using Core-compatible Quark on the host, and discard stale jobs.
3. Expose one GPU choice with automatic selection, a low default workload,
   a useful diagnostic if a driver/device fails, and the existing CPU
   option. Do not silently switch an opted-in GPU job to CPU, change fan or
   clock settings, or install a driver. Stop on user request, dialog close,
   wallet exit and device errors.
4. Test accepted/rejected shares, at least one isolated block and payout,
   reconnect and cleanup on both vendors. Then exercise the exact Windows
   and Ubuntu packages, including no-driver and CPU-only behavior, sign
   those exact packages and only then update the public download links.

[sgminer]: https://github.com/nicehash/sgminer/tree/002021095feb64065c902863cf717af7f94854bd
