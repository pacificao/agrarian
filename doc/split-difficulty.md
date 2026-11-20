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

### Required behavior with either proof available

- An honest PoW-only network and an honest PoS-only network must each be
  able to process a normal transfer and keep advancing; do not require a
  quota, alternating proofs, or an online quorum from the missing stream.
- When a proof returns, it must add evidence to the same chain. Nodes with
  the same fully validated history must reach the same decision regardless of
  message arrival order, clocks, or their local peer list.
- Blocks from both proofs should strengthen a common branch. Low
  participation is still a security risk: continued block production is not
  equivalent to finality, and two isolated groups cannot both beat a
  competing history after reconnecting. Wallet/explorer confirmation policy
  must not claim otherwise.

**Activation blocker: hybrid chain selection.** The daemon's existing branch comparator
uses accumulated `nChainWork`, calculated from the inverse compact target
without a proof-type conversion. At the captured mainnet tip (8,889), an
independent calculation of the candidate gave PoW `1d173c32` and PoS
`1c04bc95`. Under the unchanged comparator, those targets give one PoS
block about 1,256 times the score of one PoW block at that boundary.
This is a branch-selection score, not a comparison of the actual resource
costs of the proofs. Do not set an activation height until the weight
policy and its impact on reorgs and chain selection have been reviewed and
validated, including headers arriving before full block proof types are known.

### Decision: stage separate evidence with fixed reference units

Keep the proof target histories separate. For competing fully validated branches,
sum inverse-target evidence separately by proof. Compare the exact integer score
`work_total * stake_reference + stake_total * work_reference`, where the two
references must be fixed and shared by every node. Each proof adds positive
evidence without a quota, vote, or anchor from the other. Blocks on a shared
prefix cancel out. Exact arithmetic avoids rounding out low-score blocks, and
changing peer populations does not change how a node scores the same history.

Use the two captured targets above **only as staging reference fixtures**. At
those specific targets, one block of either proof contributes the same score.
For example, 24 reference PoW blocks would lose to one reference PoS block under
raw `nChainWork`; normalized scoring ranks the 24-block branch higher. This is
an explicit calibration decision, not a claim that either proof has the same
cost to produce, that either always gets half the score, or that this rule
guarantees finality. A doubled PoW reference can reverse the ranking of a
three-PoW versus two-PoS branch. Retargets and participation move contributions
over time even when the references stay fixed. The model adds no arbitrary
per-block score cap or fixed positive minimum.

The executable model in `test/functional/test_hybrid_branch_model.py` implements
that comparison, including exact ties. It does not choose a winner when
distinct branch tips have equal scores, and it does not change the daemon.
Retain raw `nChainWork` for legacy/header uses until those call sites have been
audited. For native staging, derive a separate wide score from full-block data,
including reindex/reload; never rank a header-only PoS candidate as if its proof
were known. Check that the selected score also governs any download gates that
could otherwise prevent a node from finding the better fully validated chain.
Test ties (including withholding an equal-score competing tip), restart,
reordering, and reorgs across the activation boundary. Evaluate production
reference ranges, sustained underparticipation, calibration drift, and PoW
sunset separately. Do not activate on public networks until those results and
the production fork-choice parameters have been independently reviewed.

**Separate arithmetic gate:** the legacy stake kernel multiplies the base
target by the input weight in 256 bits. The split retarget can reach high
bootstrap targets, making wraparound relevant: increasing eligible weight
must not shrink the accepted hash range. The staging candidate uses a wide
comparison for proposing and validating hybrid blocks only after the split
activates, and keeps the historical result before activation. Wide arithmetic
is not a substitute for appropriate PoS bootstrap bounds: evaluate the
acceptance rate for realistic input sizes and at the PoW sunset before
setting public activation.

References for review: [Peercoin RFC 0030](https://github.com/peercoin/rfcs/blob/master/text/0030-PoW-ASERT/0030-PoW-ASERT.md)
separates retargets but assigns PoW and PoS different security roles;
[Decred requires stakeholder votes](https://docs.decred.org/proof-of-stake/overview/)
to extend a mined chain; and [Minotaur](https://arxiv.org/abs/2201.11780)
uses mined endorsements to distribute future block-production rights rather
than allowing those mined blocks to extend the main chain directly.
None meets Agrarian's independent-liveness requirement as a drop-in
fork-choice rule.

### Reproduce the staging checks

After building the native daemon, CLI, and unit tests:

```sh
src/test/test_agrarian --run_test=main_tests
python3 -B test/functional/test_hybrid_branch_model.py -v
python3 -B test/functional/split_difficulty.py
```

The peer fixture uses fresh disposable regtest wallets, old funding
timestamps set through regtest RPC, and actual transactions. No production
wallet is necessary. Its competing-tail check covers convergence after a
shared mixed-proof prefix, not a security claim about crossing PoW/PoS
evidence on competing tails.

### Before changing mainnet

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
