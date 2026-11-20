# Independent hybrid retarget — staging candidate

Mainnet and public testnet activation are **disabled** in this branch. Only a
disposable regtest can opt in via `-regtestsplitdifficultyheight=<height>`.
Do not release these artifacts as a mainnet activation build.

## Proposed rule

While both proofs are allowed, the requested block type selects up to 24
non-genesis blocks of that same proof. Average their targets, use the actual
time between the newest and oldest samples, and clamp the elapsed/target
ratio and the move from the previous same-proof target to 1/3–3. With just
one matching block, retain its target. With no matching block, use the
network's configured target limit. All arithmetic is bounded and integral.

The candidate targets twice the existing spacing per stream: 20 minutes
each on mainnet, aiming for roughly 10 minutes combined when both streams
are active. This is not a promise about realized issuance or confirmation
times; for one active stream, the interval differs from the old combined
target. Review this policy and the bootstrap rule explicitly before
activation. Rewards, supply cap, permitted proof heights and the legacy
post-PoW calculation are unchanged.

Block creation and full-block validation must use the same proof selection.
Headers alone cannot identify PoS reliably and are not used to calculate a
proof-specific target in the absence of full ancestors. The legacy pre-68589
PoW tolerance does not apply when the split rule is active.

## Release gates

**Activation blocker: hybrid chain selection.** The existing branch comparator
uses accumulated `nChainWork`, calculated from the inverse compact target
without a proof-type conversion. At the captured mainnet tip (8,889), an
independent calculation of the candidate gave PoW `1d173c32` and PoS
`1c04bc95`. Under the unchanged comparator, those targets give one PoS
block about 1,256 times the score of one PoW block at that boundary.
This is a branch-selection score, not a comparison of the actual resource
costs of the proofs. Do not set an activation height until the weight
policy and its impact on reorgs and chain selection have been reviewed and
validated, including headers arriving before full block proof types are known.

1. Run the native wide-target and exact staking-burst regression tests and
   the opt-in peer test. Verify a legacy-rules peer agrees until activation
   and intentionally cannot accept at least one new-rules block.
2. Replay a fresh, immutable, canonical mainnet snapshot offline using each
   exact upgrade artifact with public activation still disabled. Compare
   tip, UTXO fingerprint, historical stakes and payment behavior. Do not
   publish before clean-OS Windows and Linux package tests.
3. In an isolated copy of the actual activation history, exercise delayed
   PoW, dormant PoS, rapid consecutive blocks, limited history, activation
   reorgs, IPC/template parity, Miningcore Stratum shares/block submission,
   and payouts. Benchmark a long run with only one proof; the other proof
   may have to traverse a long history. Review chain selection and issuance
   behavior under separated targets. Confirm miner and node upgrades.
4. Set the public activation only in a separately reviewed release commit.
   Agree on the boundary with pool and public node operators and make sure
   it is reachable without assuming the stalled chain will mine promptly.
   Publish signed checksum manifests and tested wallet packages ahead of
   activation. Keep wallet-serving daemons at `-staking=0`.
5. Upgrade daemons one at a time, verify wallet ownership and failover, and
   confirm agreement through the actual activation block and both proof
   types afterward. Preserve wallet backups and the original binaries and
   indexes. Before activation, rollback may restore legacy behavior after
   reconciling wallet data. After activation, switching validators back to
   legacy rules is not a safe rollback: pause and coordinate network recovery.
