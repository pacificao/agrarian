# Agrarian Core 2.0.2 — compatibility release candidate

This candidate keeps the existing mainnet and public testnet consensus rules.
It does **not** activate independent PoW/PoS difficulty or normalized fork
choice on either public network. There is no required upgrade height.

- The Windows installer uses Agrarian branding.
- Operators can exercise independent difficulty, wide stake arithmetic, and
  normalized fork choice on isolated regtest using
  `-regtestsplitdifficultyheight`. The option is rejected on public networks.

The intended platforms are Windows x64 and Ubuntu 24.04 x64. Back up your
wallet, shut down the previous version cleanly, and verify the release’s
signed checksum manifest before installing. An OpenPGP-signed checksum is
not a Windows Authenticode signature. See the [release checklist](desktop-wallet-release.md).
