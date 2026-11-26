# Agrarian Core 2.1.8

This desktop-wallet update improves pool mining controls and visibility.
It does not change consensus rules, wallet balances, staking, or pool
payouts. No daemon upgrade is required.

- The mining log now distinguishes pool authorization, waiting for a job,
  hashing, and a Thermal Guard pause. For CPU mining, it shows how many
  selected threads are hashing without waiting for an accepted share.
- On Windows, users can lift the default one-thread cap for the system
  temperature estimate for one run. The estimate is not CPU temperature;
  Thermal Guard still throttles or pauses if it gets too warm or goes missing.
- Advanced users can disable the wallet's Thermal Guard separately for CPU
  and GPU pool mining. Both switches are off by default, require confirmation,
  and reset after mining stops. With the guard off, the wallet does not
  monitor temperature, throttle, or pause that device for heat. Operating
  system, driver, and hardware protections still apply. Use independent
  temperature monitoring and adequate cooling.

Back up your wallet and exit Agrarian Core normally before upgrading. The
Windows installer is not Authenticode-signed. Keep endpoint protection
enabled and verify your download with the OpenPGP-signed checksum file.

Release signing key fingerprint: `4F4F AD72 FB96 8937 E91F B946 A431 01DB 0F93 B0EA`.

The Linux package is built on Ubuntu 24.04 for x86-64 systems. Extract it
and run `./agrarian/bin/agrarian-qt`. See the
[build and runtime guide](https://github.com/pacificao/agrarian-core/blob/v2.1.8/doc/build-wallets.md)
for runtime dependencies and verification instructions.
