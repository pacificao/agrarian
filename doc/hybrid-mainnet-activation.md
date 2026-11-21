# Mainnet hybrid activation runbook

**Not an activation notice.** Block **10,000** is a planning target only. The
current binaries use existing public rules. Never pass the regtest options to
a production daemon, change consensus on one server at a time, or infer from
the version number that a node has activated.

## Before building an activation release

1. Complete both normal and delayed-modifier multi-peer rehearsals. Require
   agreement on a competing branch with a branch-local stake origin, on a tie,
   and after restart and reindex. Also verify accumulator stake handling and
   modifier metadata reconstruction with public-network rules. No live wallet
   keys are necessary for these tests.
2. Calibrate the PoS bound and the fixed fork-score references independently.
   Run both-only and shared participation, low participation, proof return,
   and PoW sunset cases. Do not treat observed wallet balances as participating
   stake. Compare results to the [offline snapshot report](split-difficulty.md).
3. In **one reviewed source commit**, enable every new public rule using the
   same activation boundary: independent targets, kernel and branch context,
   normalized fork selection, and candidate discovery. Verify the new binary
   agrees with the old binary through the parent of the activation block and
   rejects the wrong post-activation targets. Keep the public testnet and
   regtest override policies separate from production configuration.
4. Build the exact tagged Ubuntu daemon and Windows/Ubuntu Qt packages, verify
   compatibility and signed checksums, replay the immutable mainnet prefix,
   and rehearse the real Miningcore configuration against disposable wallets.
   Verify public daemons and explorer use the proper proof-specific targets.

## Rollout and observation

1. Inventory every authoritative relay, daemon, pool endpoint and wallet
   operator that needs to upgrade. Record the common best block and tested
   release hash; do not assume that every peer is under operator control.
2. Require the full rehearsal and a sufficient block-height upgrade window.
   If there is no credible window before 10,000, move the height in source
   **before** signing an activation release. If the legacy chain stalls, an
   activation at 10,000 cannot make the intervening legacy blocks arrive.
3. Deploy the relay/fallback first while the old rule still applies. Verify
   restart, peers, common chain and web wallet ownership. Then deploy the
   primary and pool-facing services. Keep wallet-serving services on explicit
   `-staking=0`; never use custody keys to advance the chain. Preserve scoped
   wallet backups, versioned executables and the old service configuration.
4. Activate by reaching the agreed block, not by restarting all nodes at once
   or flipping runtime switches. Compare block hash and validity across nodes
   at the boundary. Verify Miningcore template and share handling, observed
   confirmations, and both proof types when present. A running service without
   chain agreement is not a successful rollout.
5. Before activation, a tested binary rollback can preserve current wallet
   data. After activation, do not revert just one validator to legacy rules.
   If agreement breaks, suspend affected wallet writes and pool submissions,
   retain all state and logs, and coordinate recovery across the network.
