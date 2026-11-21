# Agrarian Core 2.1.0 — mainnet activation candidate

**Review build only until signed and announced.** This candidate schedules a
consensus transition on Agrarian **mainnet at block 10,000**. It does not
change public testnet or enable anything on existing nodes that have not
installed it. Unreleased artifacts must not be used with a live data directory.

- PoW and PoS keep independent difficulty histories from the transition.
  The proposed starting targets are `1d216ade` (PoW) and `1c010000` (PoS).
- Fork choice combines normalized proof contributions. Either proof can
  extend the chain without the other. Transparent stakes are checked against
  the candidate branch's own unspent outputs.
- `getblockchaininfo` reports the scheduled height and when the split targets
  and normalized fork choice are in use. The old rules still apply before
  the scheduled block.

See the [activation checklist](hybrid-mainnet-activation.md) for the package,
pool, operator, and coordinated recovery gates. Replacing a validator with
an old binary is not a safe unilateral rollback after activation.
