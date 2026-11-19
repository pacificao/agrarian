Agrarian 2.0 Modernization Status
=================================

This branch is the modernization line for Agrarian. The goal is to preserve
network behavior while making the codebase build repeatably on current systems.

Verified Build Baseline
-----------------------

Use Ubuntu 24.04 as the primary build host.

The current deterministic `depends/` baseline is:

- Qt 6.8.3 LTS
- OpenSSL 3.5.6 LTS
- Boost 1.91.0
- Expat 2.8.0 for the Linux Qt/font stack
- FreeType 2.14.3 for Qt 6 static font support
- Berkeley DB 4.8.30 for portable legacy wallet compatibility
- qrencode 4.1.1 for Qt wallet QR code support
- libevent 2.1.12-stable
- GMP 6.3.0
- MiniUPnPc 2.3.3 when UPnP support is enabled
- ZeroMQ 4.3.5 when enabled

Obsolete BIP70 payment request support was removed to eliminate the protobuf
and `protoc` dependency from the wallet build. Standard `agrarian:` URI payment
links are still supported.

Berkeley DB 4.8.30 is intentionally retained for legacy `wallet.dat`
compatibility. Future wallet-storage modernization should add a dedicated SQLite
backend and migration flow instead of changing the BDB version in-place.

Hybrid release gates
--------------------

1. Keep the staking consensus patch separate from dependency, build-tree and
   wallet-format modernization. First build a source-only candidate in a clean
   tree and run the subsidy unit tests and
   `python3 test/functional/testnet_hybrid.py`. The latter uses two isolated
   testnet nodes and checks matching modifiers, a peer-accepted PoS block with a
   10 AGR subsidy, and continued PoW mining from an RPC template. Also exercise wallet encryption,
   backup/restore and restart, and independently validate masternode payments
   when they are enabled; the two-node hybrid test has no active masternodes.
2. Replay an immutable copy of the mainnet block files with a known-good build
   and the candidate in separate offline data directories, with wallets and
   outbound connections disabled. Rebuild the block index from the raw blocks
   for the candidate rather than relying on a copied index. Record the source
   tip height and hash, both replay results, the UTXO-set digest and representative
   historical block hashes. Compare stake modifiers too: the deterministic fix
   is expected to change some historical metadata. Do not infer that an index
   created by an older binary can be reused. Decide and document the required
   reindex, mixed-version behavior, activation and rollback before deployment.
   `contrib/replay-chain-offline.py` imports raw block files into a disposable
   wallet-disabled database, verifies the expected tip, and emits a JSON report.
3. Review difficulty behavior for alternating PoW/PoS and at the end of PoW,
   and verify ordinary and masternode payouts across that boundary. Do not
   change target arithmetic, historical subsidy behavior or validation rules
   as part of tooling cleanup. Use test vectors and an explicit compatibility
   plan for any rule change.
4. On a test network, check the actual pool client against the candidate daemon:
   get a mining template after PoS, submit PoW, verify notifications, and check
   reward maturity and payouts. Check explorer and web-wallet RPC compatibility.
   The verbose `getblock` response identifies block type as `proof: pow|pos`;
   serialized blocks and the mining template response are unchanged.
5. Only after those gates pass, build daemon, CLI, and Qt artifacts from the
   same reviewed commit. Run a staged node upgrade before updating pool,
   masternode, website and wallet infrastructure. Keep staking off on shared
   service wallets while evaluating a separate encrypted staking wallet. Follow
   the [desktop wallet release checklist](desktop-wallet-release.md) for downloads.

Validation record: 2026-11-19
-----------------------------

- Isolated testnet: two nodes accepted a signed PoS block paying the 10 AGR
  subsidy, then mined/submitted a valid PoW block based on the verifier's next
  `getblocktemplate` response. Both nodes agreed on their stake modifiers.
- Focused unit suite: 13 assertions passed for subsidy boundaries and
  proof-specific masternode payee checks. Local regtest covered mining, sending,
  encryption with a locked restart, and recovery of balance and transaction
  history from a disposable wallet backup.
- Offline replay: the deployed v2.0.0 reference and the candidate rebuilt
  from the same `blk00000.dat` snapshot, each with no wallet or external peers.
  Both passed level-4 verification at height 8,681, with matching tip
  `0000005c121943a095feb808637969d780ab4fba024b42ef115fcd55bfcd4d85`
  and UTXO digest
  `c022a9cd3ae28aec1b9d0335155769efc468aa4b8d11b903372ed5f7ee39dd7f`.
  The historical block hashes sampled were unchanged; the height-3 modifier
  differed, as expected. Rebuild the full block index for staking-capable
  upgraded nodes rather than carrying forward the legacy modifier index.
- Still open: mixed-version policy and canary rollout, end-to-end Miningcore
  Stratum/share/payout testing, live masternode nomination/enforcement in a
  test deployment, and clean OS checks for the packaged Windows and Linux
  desktop wallets. The pool RPC and payee checks above are not substitutes
  for these. This record is not approval to change mainnet or publish packages.

Extended staging checks
-----------------------

- Mixed peers: the deployed v2.0.0 x86-64 daemon, run under emulation,
  and the candidate accepted the same 20 PoW setup blocks, relayed and
  accepted the candidate's signed 10 AGR PoS block, and accepted PoW again
  using the legacy peer's next mining template. This test did not exercise
  enforced masternode PoS payouts or every historical stake input. The
  height-3 modifier still differed, so this does not waive the reindex
  requirement or authorize mixed-version mainnet staking.
- Miningcore: `test/functional/miningcore_staging.py` uses two real regtest
  nodes, a temporary PostgreSQL instance, a Stratum miner and PPLNS. The
  repository-built Miningcore artifact (SHA256
  `91ece2f93a1d0f83e93524a4b37d9b8e430055a14498c9ef966108936c4ece4f`)
  accepted and stored the share, submitted an accepted block, and paid
  50 test AGR to a separate wallet after coinbase maturity. A different,
  older local build accepted the same class of submissions as shares but
  did not submit the block. Identify and hash the pool binary before rollout.
- Active masternode payouts are still unverified. The isolated, regtest-only
  fixture funded six mature 10,000 AGR collaterals. Bulk startup reported
  success without registering all nodes; individual startup produced signed
  registrations that peers received, but the rehearsal did not establish a
  six-node enabled voting quorum or an enforced payout. Before release,
  inspect bulk registration and the transition from pending to enabled,
  including acceptance of the signed maturity ping. The staging fixture uses
  an extended regtest mining window and an ephemeral regtest-only signer;
  never ship the fixture daemon or use a production key to run it.

The currently verified binaries are:

- Ubuntu daemon and command-line tools
- Ubuntu Qt desktop wallet
- Windows x86_64 Qt desktop wallet and command-line tools cross-compiled from
  Ubuntu

Recommended Commands
--------------------

Native Ubuntu daemon:

    JOBS=1 ./contrib/build-linux.sh

Native Ubuntu desktop wallet:

    JOBS=1 ./contrib/build-linux-wallet.sh

Windows x86_64 desktop wallet:

    JOBS=1 ./contrib/build-win64-wallet.sh

Use higher `JOBS` values only on hosts with enough memory. On an 8-core, 16 GB
host, `JOBS=8` is reasonable.

Verification
------------

After a native Ubuntu build, run:

    ./contrib/smoke-test-daemon.sh
    ./contrib/smoke-test-wallet.sh
    ./contrib/smoke-test-qt.sh

After a Windows cross build, confirm the expected PE binaries:

    file src/qt/agrarian-qt.exe src/agrariand.exe src/agrarian-cli.exe src/agrarian-tx.exe

Documentation Status
--------------------

The preferred docs are:

- `doc/build-ubuntu-24.md`
- `doc/build-wallets.md`
- `doc/build-windows.md`
- `doc/dependencies.md`

Older macOS, RPM, Gitian, and Travis documents remain in the tree as historical
reference. They are not part of the verified 2.0 modernization path unless a
document explicitly says otherwise.
