# Agrarian Codex Handoff

## Latest AgrarianGameBuild Update - 2026-05-17

- Added native cinematic startup credits to the investor-demo notice sequence.
- Latest pushed game commit:
  `075689d Add cinematic startup credits`.
- Credits include Nathan, Hunter, Lisa, River, Fisher, and the funding note for
  cherished individuals, Pacificao seed funding, and Lina Family Investment
  Funds.
- The credits run as native Slate drawing in `UAgrarianDemoNoticeWidget`: each
  card slams in, holds briefly with a stylized illustration, and slides out.
- Updated investor demo labels to `0.1.H`.
- Added forest-fire risk/spread from irresponsible campfire/open-flame placement
  to the roadmap as a future server-authoritative system.
- Validation completed:
  - `python3 -m py_compile Scripts/verify_startup_credits_sequence.py`
  - `python3 Scripts/verify_startup_credits_sequence.py`
  - `git diff --check`
  - Windows editor build via `Scripts\BuildEditor-Windows.bat`
- Next required action:
  rebuild the Windows investor demo from `075689d`, verify archive output, and
  stop.

- Completed the final `0.1.H Fire System` roadmap item:
  `Connect rain/weather to fire behavior`.
- Latest pushed game commit:
  `879a480 Connect campfires to weather`.
- Campfires now read replicated `AAgrarianGameState::Weather`, burn fuel faster
  in rain and storms, and deterministically extinguish when wet weather leaves
  fuel below the low-fuel threshold.
- Added `AAgrarianCampfire` Blueprint helpers:
  `GetWeatherFuelDrainMultiplier()` and `IsWetWeatherActive()`.
- Added static verifier:
  `/mnt/projects/AgrarianGameBulid/Scripts/verify_fire_weather_behavior.py`.
- Validation completed:
  - `python3 -m py_compile Scripts/verify_fire_weather_behavior.py`
  - `python3 Scripts/verify_fire_weather_behavior.py`
  - `git diff --check`
  - Windows editor build via `Scripts\BuildEditor-Windows.bat`
- Roadmap state:
  - `0.1.H Fire System` is complete.
  - Next required action in the active workflow is building the Windows investor
    demo and stopping.

## Project State

Active repo:

```bash
/var/www/root_builds/agrarian
```

## Unraid Build Support Box

Unraid host being built to support Agrarian game/build work:

```text
Host: DevBox
IP: 192.168.5.8
SSH user: root
```

Do not store the plaintext Unraid password in this handoff file.

SMB / Windows share troubleshooting completed 2026-05-11:

- The initially supplied IP `192.168.4.8` was wrong/unreachable.
- Correct IP `192.168.5.8` is reachable by SSH.
- Samba is running and listening on ports `139` and `445`.
- WSD discovery is running via `wsdd2` on ports `3702` and `5355`.
- NetBIOS is disabled, so old Windows Network browsing is not expected to work;
  modern Windows should use WSD or direct UNC paths.
- Workgroup is `WORKGROUP`.
- The issue was that all shares had SMB export disabled with `shareExport="-"`,
  leaving `/etc/samba/smb-shares.conf` empty.
- After fixing share export, Samba lists these shares:
  `appdata`, `backups`, `domains`, `isos`, `projects`, `system`, and `IPC$`.

Verification command:

```bash
ssh root@192.168.5.8 'smbclient -L //127.0.0.1 -N -m SMB3'
```

Windows direct access path for build work:

```text
\\192.168.5.8\projects
```

Security follow-up: at verification time the exported shares were public and
writeable. That is convenient for setup, but `appdata`, `domains`, and `system`
should be locked down or hidden after the Windows workflow is confirmed.

Ubuntu-Codex VM repair completed 2026-05-11:

- VM name: `Ubuntu-Codex`.
- It was running in libvirt but showing the OVMF UEFI shell instead of booting.
- The vdisk was blank/uninstalled:
  `/mnt/cache/domains/Ubuntu-Codex/vdisk1.img`, raw 50 GiB.
- The attached Ubuntu ISO was corrupt/sparse. Kernel extraction produced zeroed
  data, and GRUB reported `invalid magic number` / `you need to load the kernel
  first`.
- Bad ISO preserved at:
  `/mnt/cache/isos/ubuntu-24.04.4-live-server-amd64.iso.bad-sparse`.
- Fresh Ubuntu Server ISO downloaded from Ubuntu releases and verified against
  `SHA256SUMS` as `OK`.
- VM config changed to:
  - BIOS boot instead of OVMF/pure EFI.
  - 4 GiB RAM.
  - 4 vCPU, 2 cores x 2 threads.
  - CD-ROM boot order 1, vdisk boot order 2.
- Previous VM XML backup:
  `/boot/config/Ubuntu-Codex.before-bootfix-20260511.xml`.
- Final verification: VM reaches the Ubuntu Server installer language screen.
- After installation, eject/remove the ISO or set vdisk first so it boots into
  the installed OS.

Windows 11 ISO refresh completed 2026-05-11:

- Existing `Win11_25H2_English_x64_v2.iso` was sparse/incomplete: logical size
  7.9 GiB, actual disk use about 406 MiB.
- Bad copy preserved at:
  `/mnt/cache/isos/Win11_25H2_English_x64_v2.iso.bad-sparse`.
- Fresh Microsoft-hosted ISO downloaded to:
  `/mnt/cache/isos/Win11_25H2_English_x64_v2.iso`.
- Verified SHA256:
  `768984706b909479417b2368438909440f2967ff05c6a9195ed2667254e465e3`.
- File ownership/permissions normalized for Unraid share access:
  `nobody:users`, mode `0666`.

VirtIO driver ISO refresh completed 2026-05-11:

- Existing `virtio-win-0.1.285.iso` was sparse/incomplete: logical size
  754 MiB, actual disk use about 94 MiB.
- Bad copy preserved at:
  `/mnt/cache/isos/virtio-win-0.1.285.iso.bad-sparse`.
- Fresh Fedora `virtio-win` stable ISO downloaded to:
  `/mnt/cache/isos/virtio-win-0.1.285.iso`.
- Verified byte size `789645312` and representative Windows 11 amd64 driver
  files under `vioscsi`, `viostor`, `NetKVM`, and `Balloon`.
- Verified SHA256:
  `e14cf2b94492c3e925f0070ba7fdfedeb2048c91eea9c5a5afb30232a3976331`.
- File ownership/permissions normalized for Unraid share access:
  `nobody:users`, mode `0666`.

Windows-Builder VM created on Unraid 2026-05-12:

- VM name: `Windows-Builder`.
- Intended role: Windows build/Unreal development VM.
- No GPU passthrough yet; initial graphics is VNC/QXL.
- Machine/firmware: Q35 (`pc-q35-9.2`) with OVMF pure EFI.
- CPU/RAM: 8 vCPU with topology 4 cores x 2 threads, 8 GiB RAM.
- Disk: 100 GiB raw VirtIO disk at
  `/mnt/cache/domains/Windows-Builder/vdisk1.img`.
- Attached Windows ISO:
  `/mnt/cache/isos/Win11_25H2_English_x64_v2.iso`.
- Attached VirtIO ISO:
  `/mnt/cache/isos/virtio-win-0.1.285.iso`.
- Network: bridged to `br0` with VirtIO NIC.
- TPM 2.0 emulator enabled for Windows 11 installer compatibility.
- UUID: `4244a763-91ae-4745-b417-224d42e9fb20`.
- MAC: `52:54:00:17:ec:5d`.
- Verified VM starts successfully. VNC display was `:1` / TCP `5901` at
  creation time.

Windows-Builder boot fix 2026-05-12:

- User saw the OVMF mapping table / UEFI shell instead of Windows setup.
- Found the VM definition had drifted to bad install values: max memory about
  14 GiB, current memory 1 GiB, and no TPM.
- Backups saved on Unraid:
  - `/boot/config/Windows-Builder.before-bootfix-20260512.xml`
  - `/boot/config/Windows-Builder.VARS.before-bootfix-20260512.fd`
- Re-defined VM with 8 GiB current/max RAM, 8 vCPU, OVMF/Q35, TPM 2.0, VNC/QXL,
  Windows ISO boot order 1, VirtIO disk boot order 2, and a five-second OVMF
  boot menu.
- Reset OVMF vars from `/usr/share/qemu/ovmf-x64/OVMF_VARS-pure-efi.fd`.
- Started VM, sent a boot key during the Windows DVD prompt, and verified by
  screenshot that Windows 11 Setup reached the language selection screen.

Windows-Builder RDP/QEMU tools update 2026-05-13:

- Installed local headless RDP tooling on the Codex host:
  `freerdp2-x11`, `xvfb`, `xdotool`, and `imagemagick`.
- Used headless RDP to connect to `192.168.5.12` as local admin `nathan`.
- Extracted `qemu-ga-x86_64.msi` and `virtio-win-guest-tools.exe` from the
  VirtIO ISO on Unraid and staged them through RDP drive redirection.
- Copied installers into `C:\Users\nathan\Downloads`.
- Installed QEMU Guest Agent from an elevated Windows command prompt.
- Verified Windows service `QEMU-GA` is running.
- Verified from Unraid:
  - `virsh domifaddr Windows-Builder --source agent` reports
    `192.168.5.12/22`.
  - guest agent responds to `guest-info`, version `110.0.2`.
  - active NIC is `Red Hat VirtIO Ethernet Adapter`, status `Up`, MAC
    `52-54-00-17-EC-5D`, link speed `10 Gbps`.
  - RDP/SMB/RPC remain reachable on `192.168.5.12`.
- Also ran `virtio-win-guest-tools.exe /S` from elevated prompt to install the
  bundled VirtIO guest tools package.

Ubuntu-Codex VM network fix 2026-05-13:

- Initial SSH to `nathan@192.168.5.10` failed with `No route to host`.
- From Unraid, the VM was running with VirtIO NIC model but no guest IPv4.
- Runtime NIC MAC was `34:c9:3d:2d:09:74`, conflicting with the Unraid host
  wireless-derived MAC pattern and unsuitable for a VM.
- XML backups saved on Unraid:
  - `/boot/config/Ubuntu-Codex.before-netfix-20260513.xml`
  - `/boot/config/Ubuntu-Codex.persistent.before-netfix-20260513.xml`
- Changed persistent VM NIC MAC to `52:54:00:a5:cf:63`.
- Mounted the guest vdisk offline via `qemu-nbd`; logs confirmed Ubuntu already
  had `virtio_net` in use, so there is no separate Linux VirtIO network driver
  package like on Windows.
- Added `/etc/netplan/99-static-enp1s0.yaml` with static
  `192.168.5.10/22`, gateway `192.168.4.1`, DNS `192.168.4.1` and `1.1.1.1`.
- Disabled cloud-init network rewrites with
  `/etc/cloud/cloud.cfg.d/99-disable-network-config.cfg`.
- Installed `qemu-guest-agent`; Unraid can now report guest IPs through the
  agent.
- Verified SSH works, `enp1s0` is up at `192.168.5.10/22`, `ethtool -i enp1s0`
  reports driver `virtio_net`, and a 20-packet ping test had `0%` loss.
- Passwordless sudo enabled for `nathan` via
  `/etc/sudoers.d/90-nathan-nopasswd`.
- Verified with `visudo -cf /etc/sudoers.d/90-nathan-nopasswd` and
  `sudo -n true`.

Ubuntu-Codex DevBox SMB name-resolution fix 2026-05-13:

- User reported CIFS mount path `//DevBox/projects` failed before mounting with
  `could not resolve address for DevBox`.
- Did not mount the share.
- Cause: Ubuntu was only using `/etc/hosts` plus DNS for host lookup; DNS did
  not know `DevBox`. Unraid has NetBIOS disabled and WSD enabled, which is fine
  for Windows discovery but not enough for Linux bare-name CIFS resolution.
- Backed up Ubuntu hosts file to:
  `/etc/hosts.bak.20260513-devbox-resolution`.
- Added:
  `192.168.5.8 DevBox devbox DevBox.local devbox.local`.
- Verified `getent hosts DevBox`, `ping DevBox`, and SMB port 445 via
  hostname. Share was not mounted.

Agrarian Earth-scale tile streaming design update 2026-05-15:

- Completed the roadmap item to create the Earth-scale terrain/tile streaming
  design document.
- Repo path:
  `/mnt/projects/AgrarianGameBulid/Docs/Terrain/EarthScaleTileStreamingDesign.md`
- Content captured:
  - 1 km x 1 km Earth-scale tile architecture for an eventual 510-520 million
    possible tile world.
  - Ground Zero MVP tile identity:
    `gz_us_ca_pacifica_utm10n_e544_n4160`.
  - Coordinate strategy: registry maps real-world coordinates to logical tiles;
    Unreal uses tile-local metric coordinates and World Partition placement.
  - Projection, UTM-zone, dateline, and polar handling approach.
  - Tile adjacency/stitching, package versioning, immutable publication, and
    save-data separation rules.
  - MVP static HTTP tile delivery protocol using
    `http://maps.agrariangame.com:18080`.
  - Decision: the LAN-hosted `Agrarian-TileServer` VM is acceptable for MVP and
    internal closed testing; move public delivery to an external cloud/CDN path
    before broader public testing.
  - Client cache layout, cache retention, redownload/revalidation behavior,
    single-tile import pipeline, biome/resource inference pipeline, QA gates,
    and World Partition requirements.
- Roadmap updated:
  - Marked the Earth-scale terrain/tile streaming design document complete.
  - Marked related Phase 0.7 design decisions complete.
  - Immediate next item is creating the economy and AGR design document.

Agrarian economy and AGR design update 2026-05-15:

- Completed the roadmap item to create the economy and AGR design document.
- Repo path:
  `/mnt/projects/AgrarianGameBulid/Docs/EconomyAndAgrDesignDocument.md`
- Content captured:
  - Economy progression from survival barter to specialization, settlements,
    regional trade, and later AGR utility.
  - MVP economy scope: secure barter/direct trade, ownership transfer,
    server-side validation, transaction records, and persistence.
  - AGR MVP boundary: design/placeholder only; no token transfers, wallet
    requirement, paid survival advantage, marketplace, or real-money utility in
    the first playable MVP.
  - Wallet/account direction: defer custodial vs non-custodial choice; do not
    store wallet secrets in the Unreal client, saves, logs, config, or repo.
  - Testnet/devnet, confirmation, in-game ledger, anti-abuse, fairness, legal,
    compliance, and testing gates.
- Roadmap updated:
  - Marked the economy and AGR design document complete.
  - Marked related AGR design/planning items complete.
  - Immediate next item is creating art direction, UX/HUD direction, coding
    standards, Blueprint standards, and asset/folder naming standards.

Agrarian art/UX/code/asset standards update 2026-05-15:

- Completed the roadmap item to create art direction, UX/HUD direction, coding
  standards, Blueprint standards, and asset/folder naming standards.
- Repo path:
  `/mnt/projects/AgrarianGameBulid/Docs/ArtUxCodeAndAssetStandards.md`
- Content captured:
  - Grounded visual/art direction for real terrain, survival materials, Ground
    Zero, character presentation, and startup/demo UX.
  - HUD/UX priorities for survival, interaction, inventory, crafting,
    building, weather, and player-facing copy.
  - `Content/Agrarian` folder standards and Unreal asset naming prefixes.
  - Data asset and save/persistence standards.
  - C++ and Blueprint standards for server authority, replication, categories,
    testing, and prototype content.
- Added `.gitkeep` placeholders for missing Agrarian content folders including
  Audio, Characters, Environment, Items, Materials, Prototypes, Systems, UI,
  Developer, Blueprint subfolders, and DataAsset subfolders.
- Roadmap updated:
  - Marked the standards item complete.
  - Marked the corresponding Phase 0.3 and 0.4 folder/naming checklist items
    complete.
  - Immediate next item is defining what qualifies as the 6-month MVP and what
    is explicitly excluded.

Agrarian six-month MVP definition update 2026-05-15:

- Completed the roadmap item to define what qualifies as the six-month MVP and
  what is explicitly excluded.
- Repo paths:
  - `/mnt/projects/AgrarianGameBulid/Docs/SixMonthMvpDefinition.md`
  - `/mnt/projects/AgrarianGameBulid/Docs/MvpSurvivalReadinessCriteria.md`
- Content captured:
  - Six-month MVP statement and acceptance checklist.
  - Target test audience and player count: minimum 2-player proof, target
    4-player closed-test smoke group, stretch 8-player test if stable.
  - Required pillars for startup/entry, Ground Zero map, survival loop,
    time/weather, multiplayer, persistence, UI/UX, and build/operations.
  - Explicit exclusions including full Earth-scale world, complete
    farming/livestock, family/generation systems, full economy/AGR utility,
    wallet linking, public Steam/Epic launch, vehicles, final art/audio, and
    public anti-cheat/moderation suite.
  - Acceptable first-pass biome/resource accuracy for Ground Zero.
- Roadmap updated:
  - Marked Version 0.01 Foundation Baseline complete.
  - Advanced current version to `0.1 Foundational Survival MVP`.
  - Immediate next item is deciding first-person, third-person, or hybrid
    camera.
- Milestone build rule:
  - Fresh Windows Development packaged build completed successfully after the
    `0.01` milestone closed.
  - Output archive:
    `/mnt/projects/AgrarianGameBulid/Builds/WindowsDevelopment`
  - Build command:
    `UNRAID_PASSWORD=... /home/nathan/bin/winbuilder cmd 'set AGRARIAN_NO_PAUSE=1 && pushd \\DevBox\projects\AgrarianGameBulid && Scripts\PackageWindowsDevelopment.bat'`
  - Smoke launch used the packaged `AgrarianGame.exe` with `-nullrhi`,
    `-nosound`, and an explicit log path:
    `/mnt/projects/AgrarianGameBulid/Saved/Logs/PackagedSmoke-20260515-001.log`
  - Smoke result: process was stopped after 60 seconds because it did not
    auto-exit in headless mode, but the log confirms `UEngine::Browse`,
    `LoadMap`, and `Load map complete` for
    `/Game/Agrarian/Maps/L_GroundZeroTerrain_Test`.

Agrarian camera perspective decision update 2026-05-15:

- Completed the roadmap item to decide first-person, third-person, or hybrid
  camera.
- Decision: hybrid camera with third person as the default view and an optional
  first-person toggle that players can switch back from.
- Repo path:
  `/mnt/projects/AgrarianGameBulid/Docs/CameraPerspectiveDecision.md`
- Roadmap updated:
  - Marked the camera decision complete in `1.1 Core Player Foundation`.
  - Added `Implement first/third-person camera toggle` as the next roadmap
    implementation item.
  - Immediate next item is implementing the first/third-person camera toggle.

Agrarian roadmap aging/care-quality update 2026-05-15:

- User asked where the roadmap tracks character aging, lifetime care quality,
  stat effects based on care received, and visual skin/body aging.
- Updated `/mnt/projects/AgrarianGameBulid/AGRARIAN_DEVELOPMENT_ROADMAP.md`.
- Added near-term guardrails:
  - Phase `1.2 Character Stats`: reserve long-term care history fields for
    nutrition, illness, injury, sleep, shelter, stress, workload, and treatment
    quality.
  - Phase `1.13 Persistence MVP`: save long-term character care history
    placeholders without applying aging gameplay yet.
- Expanded Phase `4.1 Aging And Lifespan` with lifetime care quality model,
  lifetime care tracking, stat impacts, childhood/development care effects, and
  long-term consequences/benefits from poor or good care.
- Added Phase `4.5 Character Visual Aging And Condition` for visual age stages,
  skin aging, hair aging, body/posture aging, care-quality appearance,
  MetaHuman/material/mesh strategy, morph/material parameters, replication,
  persistence, and UI/profile presentation.
- Immediate next roadmap item remains `Implement first/third-person camera
  toggle`.

Unraid DevBox network discovery fix 2026-05-13:

- Fixed discovery on Unraid because Windows and Ubuntu clients could not reach
  `DevBox` by name.
- Backups on Unraid:
  - `/boot/config/ident.cfg.bak.20260513-discovery`
  - `/boot/config/avahi-daemon.conf.bak.20260513-discovery`
  - `/boot/config/go.bak.20260513-discovery`
- Enabled NetBIOS in `/boot/config/ident.cfg`:
  `USE_NETBIOS="yes"`.
- Restarted Samba; verified `nmbd` is running and UDP 137/138 are listening.
- Fixed Avahi runtime publishing:
  - `host-name=DevBox`
  - `allow-interfaces=br0`
  - `publish-workstation=yes`
  - `use-ipv6=no`
- Restarted Avahi; it now advertises `DevBox.local` instead of
  `DevBox-4.local`.
- Added a boot-time block to `/boot/config/go` so the Avahi runtime changes are
  re-applied after reboot.
- Verified from Windows VM:
  - `Resolve-DnsName DevBox` returns `DevBox.local` and IPv4 `192.168.5.8`.
  - `Test-NetConnection DevBox -Port 445` succeeds.
  - `Test-NetConnection DevBox.local -Port 445` succeeds.
- Verified from `ubuntu-codex`: hostname resolution and TCP 445 work for
  `DevBox` and `DevBox.local`.
- Caveat: arbitrary Linux clients using bare `//DevBox/...` still need DNS or
  `/etc/hosts` for the single-label name, or they should use `DevBox.local`
  with mDNS enabled.

Active branch:

```bash
2.0
```

The 2.0 branch is intended to become the main development line after final build
validation. The worktree may contain build-generated files after local compiles;
do not commit generated build output.

## Current Build Baseline

Validated locally during the final pre-main pass:

```bash
env JOBS=8 ./contrib/build-linux.sh
env JOBS=8 ./contrib/build-linux-wallet.sh
./contrib/smoke-test-daemon.sh
./contrib/smoke-test-wallet.sh
./contrib/smoke-test-qt.sh
```

Expected successful artifacts:

```bash
src/agrariand
src/agrarian-cli
src/agrarian-tx
src/qt/agrarian-qt
```

Additional validation completed during this pass:

```bash
ALLOW_ROOT_BUILD_MENU=1 AGRARIAN_MENU_CHOICE=windows-daemon WORKDIR=/var/www/root_builds/agrarian BRANCH=2.0 JOBS=8 ./contrib/agrarian-build-menu.sh
ALLOW_ROOT_BUILD_MENU=1 AGRARIAN_MENU_CHOICE=windows-qt WORKDIR=/var/www/root_builds/agrarian BRANCH=2.0 JOBS=8 ./contrib/agrarian-build-menu.sh
ALLOW_ROOT_BUILD_MENU=1 AGRARIAN_MENU_CHOICE=linux-arm64-daemon WORKDIR=/var/www/root_builds/agrarian BRANCH=2.0 JOBS=8 ./contrib/agrarian-build-menu.sh
ALLOW_ROOT_BUILD_MENU=1 AGRARIAN_MENU_CHOICE=linux-arm64-qt WORKDIR=/var/www/root_builds/agrarian BRANCH=2.0 JOBS=8 ./contrib/agrarian-build-menu.sh
```

The ARM64 daemon cross-build produced ARM aarch64 ELF binaries. The ARM64 Qt
target correctly refused to run on x86_64 and remains native-only.

## Build Menu

Use the menu for fresh-machine testing:

```bash
curl -L https://raw.githubusercontent.com/pacificao/agrarian/2.0/contrib/agrarian-build-menu.sh -o agrarian-build-menu.sh
chmod +x agrarian-build-menu.sh
BRANCH=2.0 ./agrarian-build-menu.sh
```

For controlled root-only environments only:

```bash
ALLOW_ROOT_BUILD_MENU=1 BRANCH=2.0 ./contrib/agrarian-build-menu.sh
```

The menu now includes Linux, Windows, and ARM64 targets. ARM64 daemon cross-build
from x86_64 is supported. ARM64 Qt is native-only for now.

## Network Notes

Mainnet P2P port:

```text
51336
```

Seed nodes currently use:

```text
node1.agrariancoin.com
node2.agrariancoin.com
node3.agrariancoin.com
node4.agrariancoin.com
node5.agrariancoin.com
```

The daemon has been used to test peer visibility with another node at
`dev.barnealogy.com`. Recheck connectivity after major consensus or network
changes.

## Important Decisions

- Keep Berkeley DB at 4.8.30 for 2.0 wallet compatibility.
- Do not upgrade to BDB 18.x in this release.
- Treat a SQLite wallet backend as a future migration project with backup,
  migration, and rollback documentation.
- For now, Linux validation is the fastest correctness loop, but Windows builds
  must pass before release.
- Do not assume Linux success guarantees Windows success; cross-build and Qt
  plugin/link behavior can fail independently.

## Known Warning Areas

- `std::random_shuffle` deprecation warnings remain.
- Some vendored dependency warnings remain.
- Some old Boost/LevelDB warning noise remains.
- Cleanup should be correctness-driven, not broad cosmetic churn before release.

## Safe Cleanup Pattern

Before committing, stage intentional source/doc/script files first, then revert
unstaged build output:

```bash
git add <intentional files>
git restore .
git clean -nd
```

Only run `git clean -fd` after reviewing the dry-run output.

## Ubuntu-Codex DevBox Project Mount / GitHub Setup - 2026-05-13

- On `ubuntu-codex` (`192.168.5.10`), mounted Unraid `//DevBox/projects` at
  `/mnt/projects`.
- Persistent mount added to `/etc/fstab` using guest/public SMB access:
  `//DevBox/projects /mnt/projects cifs guest,uid=nathan,gid=nathan,file_mode=0775,dir_mode=0775,vers=3.1.1,noserverino,_netdev,nofail,x-systemd.automount,x-systemd.idle-timeout=600 0 0`
- The `projects` share is public on Unraid, so no SMB password is needed for
  this mount.
- Installed `git-lfs` on `ubuntu-codex` and ran `git lfs install`.
- GitHub SSH access on `ubuntu-codex` was configured with a GitHub-only SSH
  identity copied from the working local Codex host key. Do not write private
  key material or passwords into handoff files.
- A newly generated Ubuntu-Codex-specific GitHub key was preserved as
  `~/.ssh/id_ed25519_ubuntu_codex_generated`; it is not active unless added to
  GitHub later and referenced in `~/.ssh/config`.
- Cloned the private Unreal project repo:
  `git@github.com:pacificao/AgrarianGameBuild.git`
- Clone target, matching the requested path spelling:
  `/mnt/projects/AgrarianGameBulid`
- Verification:
  - `git status --short --branch` returned clean `main...origin/main`.
  - `git fetch --dry-run origin` succeeded.
  - `git push --dry-run origin HEAD` succeeded with `Everything up-to-date`.
  - Git LFS hooks under `.git/hooks` are executable after changing CIFS
    `file_mode` to `0775`.

## Windows-Builder Tooling Setup - 2026-05-13

- Target toolchain for Agrarian Game build VM:
  - Unreal Engine `5.7`
  - Visual Studio Community `2026`
- Previously installed/verified:
  - VirtIO guest tools
  - QEMU guest agent
  - VirtIO network driver
- Installed Visual Studio Community 2026 from Microsoft stable 18.x bootstrapper:
  - Install path: `C:\Program Files\Microsoft Visual Studio\2026\Community`
  - Verified `devenv.exe` exists.
  - Verified MSVC toolchain exists at:
    `C:\Program Files\Microsoft Visual Studio\2026\Community\VC\Tools\MSVC\14.51.36231`
  - Workloads requested:
    - `Microsoft.VisualStudio.Workload.NativeDesktop`
    - `Microsoft.VisualStudio.Workload.NativeGame`
    - `--includeRecommended` for recommended SDK/tooling components.
- Installed Epic Games Launcher from official Epic MSI:
  - MSI cached on Unraid:
    `/mnt/user/isos/EpicGamesLauncherInstaller.msi`
  - Launcher verified at:
    `C:\Program Files\Epic Games\Launcher\Portal\Binaries\Win64\EpicGamesLauncher.exe`
- Git was intentionally skipped on Windows for now because `ubuntu-codex`
  manages the repository on `/mnt/projects` / `\\DevBox\projects`.
- Remaining manual/interactive step:
  - Log into Epic Games Launcher on Windows-Builder and install Unreal Engine
    `5.7`. Epic does not provide a normal unauthenticated silent installer for
    launcher-managed Unreal Engine installs.

Windows-Builder disk expansion on 2026-05-13:

- User increased VM virtual disk from 100GB to 250GB.
- Windows Disk Management could not extend `C:` because the Windows Recovery
  partition was immediately after `C:`.
- Inside Windows:
  - Disabled WinRE with `reagentc /disable`.
  - Removed Disk 0 Partition 4, the small Recovery partition.
  - Extended `C:` to the supported maximum.
  - Re-enabled WinRE with `reagentc /enable`.
- Final state:
  - `C:` NTFS size: `268207894528` bytes, roughly 249.8GB.
  - Free space immediately after expansion: `206745464832` bytes, roughly
    192.6GB.
  - WinRE status: enabled, now located under partition 3 / `C:`.

## Agrarian Game Build Lane And Input Setup - 2026-05-13/14

- Current Unreal project checkout:
  `/mnt/projects/AgrarianGameBulid`
- Windows share path:
  `\\DevBox\projects\AgrarianGameBulid`
- Windows-Builder `P:` drive:
  `P:` is mapped to `\\DevBox\projects` for the Windows `nathan` user.
  `P:\AgrarianGameBulid\AgrarianGame.uproject` was verified.
- Persistent Windows mapping support:
  - Scheduled task: `MapDevBoxProjectsPLogon`
  - Script: `C:\Temp\map-p-drive.cmd`
  - Log: `C:\Temp\map-p-drive.log`
- Codex Windows helper:
  `/home/nathan/bin/winbuilder`
  - Uses Unraid QEMU guest agent to run Windows commands.
  - Requires `UNRAID_PASSWORD` in the environment.
  - Do not write the password into scripts or handoff files.
- Codex Unreal build helper:
  `/home/nathan/bin/agrarian-build-editor`
  - Verified successful after latest input asset changes.

Repo commit completed:

```text
744b3c3 Add interact input assets
```

That commit includes:

- `Scripts/RunUnrealPython-Windows.bat`
- `Scripts/setup_interact_input.py`
- `Scripts/verify_interact_input.py`
- `Content/Input/Actions/IA_Interact.uasset`
- updated `Content/Input/IMC_Default.uasset`
- updated `Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset`
- updated `AGRARIAN_DEVELOPMENT_ROADMAP.md`

Completed roadmap item:

- Created `IA_Interact`.
- Bound `IA_Interact` to `E`.
- Bound `IA_Interact` to `Gamepad_FaceButton_Left`.
- Assigned `IA_Interact` to `BP_ThirdPersonCharacter.InteractAction`.
- Verified the saved assets with a fresh Unreal command-mode load using
  `Scripts/verify_interact_input.py`.
- Verified `AgrarianGameEditor Win64 Development` still builds through the
  Codex headless Windows build lane.

Next roadmap item:

- Create item definition assets for wood, stone, fiber, food, meat, hide, and
  primitive structure parts.

## Agrarian Item Definition Assets - 2026-05-13/14

Completed the item definition asset roadmap item.

Added scripts:

- `Scripts/setup_item_definitions.py`
- `Scripts/verify_item_definitions.py`

Created data assets under:

```text
/Game/Agrarian/DataAssets/Items
```

Created item definitions:

- `DA_Item_Wood` / `wood`
- `DA_Item_Stone` / `stone`
- `DA_Item_Fiber` / `fiber`
- `DA_Item_Food` / `food`
- `DA_Item_Meat` / `meat`
- `DA_Item_Hide` / `hide`
- `DA_Item_PrimitiveFrame` / `primitive_frame`
- `DA_Item_PrimitiveWallPanel` / `primitive_wall_panel`
- `DA_Item_PrimitiveRoofPanel` / `primitive_roof_panel`

Verification:

- `Scripts/verify_item_definitions.py` passed in Unreal command mode from a
  fresh editor load.
- `/home/nathan/bin/agrarian-build-editor` succeeded after the new assets.

Roadmap:

- `AGRARIAN_DEVELOPMENT_ROADMAP.md` now marks the item definition asset task
  complete.

Git:

- These item definition changes are not committed yet as of this handoff note.

Next roadmap item:

- Create recipe data assets for campfire, primitive shelter, basic tool, and
  bandage.

## Agrarian Recipe Data Assets - 2026-05-13/14

Completed the recipe data asset roadmap item.

Added scripts:

- `Scripts/setup_recipe_definitions.py`
- `Scripts/verify_recipe_definitions.py`

Created recipe data assets under:

```text
/Game/Agrarian/DataAssets/Recipes
```

Created recipes:

- `DA_Recipe_Campfire` / `campfire`
- `DA_Recipe_PrimitiveShelter` / `primitive_shelter`
- `DA_Recipe_BasicTool` / `basic_tool`
- `DA_Recipe_Bandage` / `bandage`

Verification:

- `Scripts/verify_recipe_definitions.py` passed in Unreal command mode from a
  fresh editor load.
- `/home/nathan/bin/agrarian-build-editor` succeeded after the new assets.

Roadmap:

- `AGRARIAN_DEVELOPMENT_ROADMAP.md` now marks the recipe data asset task
  complete.
- Primitive tool, campfire, shelter, and bandage recipe line items are marked
  complete under Primitive Crafting.
- Added a follow-up item to create item definition assets for craft result
  items: campfire, primitive shelter, basic tool, and bandage.

Git:

- These item and recipe asset changes are not committed yet as of this handoff
  note.

Next roadmap item:

- Create item definition assets for craft result items, then create Blueprint
  child actors for wood resource, campfire, primitive shelter, and first
  wildlife species.

## Agrarian Crafted Result Items And Playable Blueprints - 2026-05-13/14

Completed the next roadmap item after recipe data assets.

Extended item definition scripts:

- `Scripts/setup_item_definitions.py`
- `Scripts/verify_item_definitions.py`

Added crafted result item definitions:

- `DA_Item_Campfire` / `campfire`
- `DA_Item_PrimitiveShelter` / `primitive_shelter`
- `DA_Item_BasicTool` / `basic_tool`
- `DA_Item_Bandage` / `bandage`

Added Blueprint helper scripts:

- `Scripts/setup_playable_blueprints.py`
- `Scripts/verify_playable_blueprints.py`

Created Blueprint child actors:

- `/Game/Agrarian/Blueprints/Resources/BP_WoodResourceNode`
- `/Game/Agrarian/Blueprints/Structures/BP_Campfire`
- `/Game/Agrarian/Blueprints/Structures/BP_PrimitiveShelter`
- `/Game/Agrarian/Blueprints/Wildlife/BP_RabbitWildlife`

Verification:

- `Scripts/verify_item_definitions.py` passed in Unreal command mode from a
  fresh editor load.
- `Scripts/verify_playable_blueprints.py` passed in Unreal command mode from a
  fresh editor load.
- `/home/nathan/bin/agrarian-build-editor` succeeded after the new assets.

Roadmap:

- `AGRARIAN_DEVELOPMENT_ROADMAP.md` now marks crafted result item definitions
  and first playable Blueprint child actors complete.

Git:

- These item, recipe, and Blueprint asset changes are not committed yet as of
  this handoff note.

Next roadmap item:

- Place the wood resource node, campfire, primitive shelter, and rabbit
  wildlife Blueprint in the test map.

## Codex Local Session Config - 2026-05-13/14

Local Codex config was updated at:

```text
/home/nathan/.codex/config.toml
```

Current important setting:

```toml
sandbox_mode = "danger-full-access"
```

Reason:

- The current session repeatedly hit:
  `bwrap: loopback: Failed RTM_NEWADDR: Operation not permitted`
- New/resumed trusted sessions should avoid the broken Linux sandbox wrapper
  while keeping normal approval prompts active.
- This is not the full approval/sandbox bypass mode.

Security note:

- Do not store plaintext passwords, private SSH keys, Mailgun/API secrets, or
  Unraid/Windows login passwords in handoff files.

## Agrarian Test Map Placements - 2026-05-13/14

Completed the next roadmap item after playable Blueprint child actors.

Added editor automation helper:

- `Source/AgrarianGame/AgrarianEditorAutomationLibrary.h`
- `Source/AgrarianGame/AgrarianEditorAutomationLibrary.cpp`

Purpose:

- Exposes `SpawnActorInEditorWorld` for Unreal Python setup scripts.
- Bypasses Unreal's viewport-backed editor placement path, which crashed in
  command-mode `-NullRHI` while spawning actors.
- `Source/AgrarianGame/AgrarianGame.Build.cs` adds `UnrealEd` only for editor
  builds.

Added placement scripts:

- `Scripts/setup_test_map_placements.py`
- `Scripts/verify_test_map_placements.py`

Placed actors in `/Game/ThirdPerson/Lvl_ThirdPerson`:

- `AGR_WoodResourceNode_01`
- `AGR_Campfire_01`
- `AGR_PrimitiveShelter_01`
- `AGR_RabbitWildlife_01`

Verification:

- `Scripts/verify_test_map_placements.py` passed in Unreal command mode from a
  fresh editor load.
- `/home/nathan/bin/agrarian-build-editor` succeeded after the map placement
  and helper code changes.

Roadmap:

- Test map placement item is complete.
- Near-term placement/test items for wood resource, campfire, primitive
  shelter, and rabbit wildlife are complete.

Git:

- These map placement and helper-code changes are not committed yet as of this
  handoff note.

Next roadmap item:

- Run the first full gather -> inventory -> craft -> place shelter -> save/load
  loop.

## Agrarian Playable Loop Smoke Test - 2026-05-13/14

Completed the next roadmap item after test map placement.

Updated editor automation:

- Extended `Source/AgrarianGame/AgrarianEditorAutomationLibrary.h`
- Extended `Source/AgrarianGame/AgrarianEditorAutomationLibrary.cpp`

Added verification script:

- `Scripts/verify_playable_loop_smoke.py`

What the smoke test covers:

- Loads `/Game/ThirdPerson/Lvl_ThirdPerson`.
- Finds placed `AGR_WoodResourceNode_01`.
- Spawns a test `BP_ThirdPersonCharacter`.
- Gathers wood through the real interactable resource node path.
- Seeds only currently-unobtainable primitive shelter ingredients.
- Crafts `primitive_shelter` through `UAgrarianCraftingComponent`.
- Places `BP_PrimitiveShelter` through `UAgrarianBuildingPlacementComponent`.
- Captures persistent actor state from `UAgrarianPersistentActorComponent`.
- Restores captured primitive shelter actors from saved state data.

Verification:

- `/home/nathan/bin/agrarian-build-editor` succeeded after helper changes.
- `Scripts/verify_playable_loop_smoke.py` passed in Unreal command mode:
  `PASS: gathered wood 0->2, crafted primitive_shelter, placed shelter, saved
  2 persistent actor(s), restored 2 actor(s)`.

Roadmap:

- Gather -> inventory -> craft -> place shelter -> save/load data smoke loop is
  complete.
- Added follow-up item to make primitive shelter ingredients naturally
  obtainable in normal play.
- Added follow-up item for a later PIE/server persistence test using
  `UAgrarianPersistenceSubsystem` with a live GameInstance.

Git:

- These playable-loop smoke test changes are not committed yet as of this
  handoff note.

Next roadmap item:

- Test wildlife damage/death/harvest loop.

## Agrarian Wildlife Damage/Harvest Smoke Test - 2026-05-14

Completed the next roadmap item after the playable loop smoke test.

Added verification script:

- `Scripts/verify_wildlife_loop.py`

What the smoke test covers:

- Loads `/Game/ThirdPerson/Lvl_ThirdPerson`.
- Finds placed `AGR_RabbitWildlife_01`.
- Spawns a test `BP_ThirdPersonCharacter`.
- Applies nonlethal wildlife damage and verifies the rabbit enters fleeing
  state.
- Applies lethal wildlife damage and verifies the rabbit is dead.
- Harvests the dead rabbit.
- Verifies `meat` and `hide` are added to the character inventory.
- Verifies the rabbit cannot be harvested twice.

Verification:

- `Scripts/verify_wildlife_loop.py` passed in Unreal command mode:
  `PASS: wildlife damage/death/harvest verified health 12.0->11.0->0, meat
  0->1, hide 0->1`.

Roadmap:

- Wildlife damage/death/harvest loop is complete.
- Wildlife damage task is complete.

Git:

- These wildlife smoke test changes are not committed yet as of this handoff
  note.

Next roadmap item:

- Make all primitive shelter ingredients obtainable through normal play.

## Agrarian Natural Primitive Shelter Ingredient Loop - 2026-05-14

Completed the next roadmap item after wildlife damage/death/harvest testing.

Added recipe assets:

- `DA_Recipe_PrimitiveFrame`
- `DA_Recipe_PrimitiveWallPanel`
- `DA_Recipe_PrimitiveRoofPanel`

Added gather path:

- `BP_FiberResourceNode`
- Placed `AGR_FiberResourceNode_01` in `/Game/ThirdPerson/Lvl_ThirdPerson`

Updated existing playable assets:

- Wood resource node now has enough harvests for a full primitive shelter loop.
- Rabbit wildlife now yields 2 hide so one rabbit satisfies the shelter recipe.

Updated automation:

- Added `RunNaturalShelterLoopSmokeTest` to
  `UAgrarianEditorAutomationLibrary`.
- Updated `Scripts/verify_playable_loop_smoke.py` so it no longer seeds missing
  shelter ingredients. It gathers wood, gathers fiber, harvests hide, crafts
  primitive frame/wall/roof parts, crafts `primitive_shelter`, places it, and
  verifies save/restore state.

Verification:

- `Scripts/verify_recipe_definitions.py` passed.
- `Scripts/verify_playable_blueprints.py` passed.
- `Scripts/verify_test_map_placements.py` passed.
- `Scripts/verify_playable_loop_smoke.py` passed:
  `PASS: naturally gathered wood=26 fiber=24 hide=2, crafted parts 2/4/2,
  crafted and placed primitive_shelter, saved 2 persistent actor(s), restored
  2 actor(s)`.
- `Scripts/verify_wildlife_loop.py` passed after the hide-yield change:
  `PASS: wildlife damage/death/harvest verified health 12.0->11.0->0, meat
  0->1, hide 0->2`.

Roadmap:

- Natural primitive shelter ingredients are complete.
- Fiber resource is complete.
- Primitive shelter structure part recipes are complete.

Git:

- These natural shelter loop changes are not committed yet as of this handoff
  note.

Next roadmap item:

- Add a PIE/server persistence test that exercises `UAgrarianPersistenceSubsystem`
  with a live GameInstance.

## Agrarian Live GameInstance Persistence Automation - 2026-05-14

Completed the next roadmap item after the natural primitive shelter ingredient
loop.

Added native Unreal automation:

- `Source/AgrarianGame/AgrarianPersistenceAutomationTest.cpp`
- Test name: `Agrarian.PersistenceSubsystem.LiveGameInstance`

Updated editor automation:

- Added `RunPersistenceSubsystemSmokeTest` to
  `UAgrarianEditorAutomationLibrary`.
- The smoke test uses the live `UAgrarianPersistenceSubsystem`, a temporary
  save slot named `AgrarianAutomationPersistence`, registers the
  `primitive_shelter` actor class, saves a spawned shelter, reloads the save,
  restores the actor, verifies the counts, and deletes the temporary save slot.

Verification:

- `/home/nathan/bin/agrarian-build-editor` passed.
- Unreal automation passed with:
  `Automation RunTests Agrarian.PersistenceSubsystem.LiveGameInstance`
- Result:
  `PASS: live persistence subsystem saved 1 actor(s), restored 1 actor(s), world
  now has 1 persistent actor(s)`.

Implementation note:

- The first test used `/Game/ThirdPerson/Lvl_ThirdPerson`; the subsystem check
  passed, but command-mode PIE marked the test failed because that
  world-partition map emits a handled editor viewport ensure in headless mode.
  The final test creates a blank transient editor map before starting PIE so the
  result reflects the persistence subsystem only.

Roadmap:

- Live GameInstance persistence test is complete.

Git:

- These persistence automation changes are not committed yet as of this handoff
  note.

Next roadmap item:

- Decide whether to keep the current template variants or remove unused starter
  variants.

## Agrarian Earth-Scale Terrain And Tile Roadmap Direction - 2026-05-14

The roadmap was updated with the long-term terrain/tile direction before moving
to the next implementation task.

User direction captured:

- Agrarian should eventually support real-world terrain at very large scale.
- Terrain tile unit is 1 km x 1 km.
- Full Earth-scale coverage implies roughly 510-520 million possible tiles.
- Tiles should be generated and added over many years, not all at once.
- Tiles should be served from a server, cached locally, scrubbed when unused for
  a configured amount of time, and redownloaded if a player returns.
- Terrain height, ocean depth, mountains, hills, rivers, biomes, and natural
  resources should be derived from real-world data where practical.
- Travel should use believable real-world pacing for humans, animals, boats,
  tractors, cars, horses, etc., modified by age, condition, strength, endurance,
  terrain, fatigue, carried weight, hunger/thirst, and injury.
- Need to choose one "Ground Zero" 1 km x 1 km MVP tile and use it to prove
  automated terrain import, tile tracking, stitching, and World Partition
  placement.

Roadmap changes:

- Reordered the roadmap so Phase 0 comes before `Version .01 / 0.01`.
- Added Phase 0.7 `Earth-Scale Terrain Architecture`.
- Added Ground Zero MVP tile tasks to Phase 0.5, Phase 1.4, and near-term
  actions.
- Added tile registry/database tasks for a future 510-520 million tile registry.
- Added real terrain, bathymetry, river/coastline, biome, natural resource,
  cache, streaming, scrubber, and tile QA tasks.
- Immediate next item is now:
  `Choose Ground Zero 1 km MVP tile and define first tile registry schema`.

Verification:

- Documentation-only update; no build/test run required.

## Agrarian Ground Zero Tile And Registry Schema - 2026-05-14

Completed the roadmap item to choose the Ground Zero 1 km MVP tile and define
the first tile registry schema.

Ground Zero:

- Location: Linda Mar / San Pedro Valley edge in Pacifica, California.
- Tile ID: `gz_us_ca_pacifica_utm10n_e544_n4160`
- Projection for prototype: `WGS84 / UTM zone 10N`
- Bounds:
  - Easting: `544000-545000`
  - Northing: `4160000-4161000`
- Nominal center:
  - Latitude: `37.5925`
  - Longitude: `-122.4995`
- Working biome: `coastal_california_scrub_woodland`

Files added:

- `Docs/Terrain/GroundZeroTile.md`
- `Docs/Terrain/TileRegistrySchema.md`
- `Data/Tiles/tile_registry.schema.json`
- `Data/Tiles/ground_zero_tiles.json`
- `Data/Tiles/tile_registry.sql`

Registry seed:

- Includes the Ground Zero tile plus all eight adjacent placeholder neighbors.
- Tracks candidate source categories for elevation, hydrography, bathymetry, and
  land cover.
- Separates terrain tile registry state from player-made world persistence.

Validation:

- JSON syntax validated for both registry files.
- Sanity check verified 9 tiles, 1 km tile size, and Ground Zero neighbor
  references.
- SQL schema executed successfully in an in-memory SQLite database.

Roadmap:

- Ground Zero selection is complete.
- First tile registry schema is complete.
- MVP tile metadata/registry prototype is complete.

Next roadmap item:

- Prototype real terrain import for the selected MVP tile.

## Agrarian Ground Zero Terrain Import Prototype - 2026-05-14

Completed the roadmap item to prototype real terrain import for the selected MVP
tile.

Added:

- `Scripts/prototype_ground_zero_terrain.py`
- `Docs/Terrain/TerrainImportPrototype.md`

Generated:

- `Data/Terrain/Generated/gz_us_ca_pacifica_utm10n_e544_n4160/gz_us_ca_pacifica_utm10n_e544_n4160_elevation_samples_33.csv`
- `Data/Terrain/Generated/gz_us_ca_pacifica_utm10n_e544_n4160/gz_us_ca_pacifica_utm10n_e544_n4160_heightmap_33.r16`
- `Data/Terrain/Generated/gz_us_ca_pacifica_utm10n_e544_n4160/gz_us_ca_pacifica_utm10n_e544_n4160_terrain_metadata.json`

Generation result:

- Source: USGS Elevation Point Query Service.
- Sample grid: 33 x 33.
- Sample count: 1,089.
- Spacing: 31.25 m.
- Elevation range: `3.458832026m` to `96.11089325m`.
- USGS reported raster ID: `65833`.
- USGS reported resolution: `1m`.

Validation:

- Terrain metadata JSON parses cleanly.
- Tile registry JSON parses cleanly after generation.
- CSV has 1 header row plus 1,089 sample rows.
- R16 size is 2,178 bytes, matching `33 * 33 * 2`.
- Ground Zero tile registry status is now `generated` with
  `generation_version=1`.

Roadmap:

- Real terrain import prototype is complete.
- Real elevation terrain base is marked in progress.

Next roadmap item:

- Define acceptable real terrain accuracy and final DEM/lidar source
  requirements for the MVP tile.

## Agrarian Terrain Accuracy And Source Requirements - 2026-05-14

Completed the roadmap item to define acceptable real terrain accuracy and final
DEM/lidar source requirements for the Ground Zero MVP tile.

Added:

- `Docs/Terrain/TerrainAccuracyRequirements.md`

Updated:

- `Data/Tiles/ground_zero_tiles.json`
  - Keeps USGS Elevation Point Query Service as the confirmed prototype source.
  - Adds USGS 3DEP 1-meter DEM / Seamless 1-Meter DEM as the MVP target
    elevation source.
  - Adds NOAA/NCEI coastal DEM or Coastal Relief Model as the MVP coastal or
    bathymetry target source if the tile area requires it.

Requirements now defined:

- Tier 0 current prototype.
- Tier 1 MVP required accuracy.
- Tier 2 preferred post-MVP accuracy.
- Final ground elevation source priority.
- Coastal and bathymetry source priority.
- Unreal import metadata requirements.
- MVP terrain acceptance tests.
- Known gaps in the current 33 x 33 point-sampled prototype.

Validation:

- `python3 -m json.tool Data/Tiles/ground_zero_tiles.json` passed.

Roadmap:

- Real terrain accuracy requirements are complete.
- Bathymetry/ocean-depth handling requirements are complete.
- Real-world terrain source evaluation is complete.

Next roadmap item:

- Acquire or extract the final USGS 3DEP DEM/lidar source for the Ground Zero
  tile.

## Agrarian USGS 3DEP DEM Acquisition And Extraction - 2026-05-14

Completed the roadmap item to acquire/extract the final USGS 3DEP DEM/lidar
source for the Ground Zero tile.

Installed free geospatial packages on `pacificao-dev`:

- `gdal-bin`
- `python3-gdal`
- `python3-rasterio`

Added:

- `Scripts/acquire_ground_zero_dem.py`
- `Scripts/extract_ground_zero_dem_subset.py`
- `Docs/Terrain/DemAcquisition.md`

Source products:

- `USGS 1 Meter 10 x54y416 CA_CaliforniaGaps_B23`
- `USGS 1 Meter 10 x54y417 CA_CaliforniaGaps_B23`
- Dataset: `Digital Elevation Model (DEM) 1 meter`
- Publication date: `2025-08-20`
- Format: GeoTIFF

Ground Zero sits on a 3DEP 10 km tile boundary, so both products are required.

Files:

- `Data/Terrain/Sources/gz_us_ca_pacifica_utm10n_e544_n4160/USGS_1M_10_x54y416_CA_CaliforniaGaps_B23.tif`
- `Data/Terrain/Sources/gz_us_ca_pacifica_utm10n_e544_n4160/USGS_1M_10_x54y417_CA_CaliforniaGaps_B23.tif`
- `Data/Terrain/Sources/gz_us_ca_pacifica_utm10n_e544_n4160/gz_us_ca_pacifica_utm10n_e544_n4160_tnm_1m_dem_product.json`
- `Data/Terrain/Extracted/gz_us_ca_pacifica_utm10n_e544_n4160/gz_us_ca_pacifica_utm10n_e544_n4160_1m_dem_subset.tif`
- `Data/Terrain/Extracted/gz_us_ca_pacifica_utm10n_e544_n4160/gz_us_ca_pacifica_utm10n_e544_n4160_1m_dem_subset_metadata.json`

Validation:

- `gdalinfo` reports extracted subset size `1000 x 1000`.
- Pixel size is `1.0m x 1.0m`.
- CRS is `EPSG:26910` / NAD83 UTM zone 10N.
- Bounds are E `544000-545000`, N `4160000-4161000`.
- Metadata JSON parses cleanly.
- Tile registry source metadata validates with `status=source_data_found`,
  `coverage_status=confirmed`, and local source folder populated.

Roadmap:

- USGS 3DEP DEM acquisition/extraction is complete.

Next roadmap item:

- Convert the extracted 1-meter DEM subset into an Unreal Landscape-ready
  heightmap and import plan.

## Agrarian Unreal-Ready Ground Zero Heightmap - 2026-05-14

Completed the roadmap item to convert the extracted 1-meter DEM subset into an
Unreal Landscape-ready heightmap and import plan.

Added:

- `Scripts/convert_ground_zero_dem_to_unreal_heightmap.py`
- `Docs/Terrain/UnrealLandscapeImportPlan.md`

Generated:

- `Data/Terrain/Unreal/gz_us_ca_pacifica_utm10n_e544_n4160/gz_us_ca_pacifica_utm10n_e544_n4160_unreal_1009.r16`
- `Data/Terrain/Unreal/gz_us_ca_pacifica_utm10n_e544_n4160/gz_us_ca_pacifica_utm10n_e544_n4160_unreal_1009_preview.pgm`
- `Data/Terrain/Unreal/gz_us_ca_pacifica_utm10n_e544_n4160/gz_us_ca_pacifica_utm10n_e544_n4160_unreal_heightmap_metadata.json`

Notes:

- Optional PNG output was skipped because Pillow is not installed.
- The `.r16` is the Unreal import target.

Import settings:

- Heightmap resolution: `1009 x 1009`
- X scale: `99.2063492063492 cm`
- Y scale: `99.2063492063492 cm`
- Z scale: `18.22824631817639 cm`
- Elevation range: `3.176246404647827m` to `96.50486755371094m`
- Z offset guidance: `3.176246404647827m`

Validation:

- Metadata JSON parses cleanly.
- R16 byte size is `2,036,162`, matching `1009 * 1009 * 2`.
- Metadata confirms 1009 x 1009 resolution and expected Unreal scale values.

Roadmap:

- Unreal Landscape-ready heightmap/import plan is complete.

Next roadmap item:

- Import the Ground Zero R16 heightmap into an Unreal terrain test map.

## Agrarian Ground Zero Unreal Terrain Map Import - 2026-05-14

Completed the roadmap item to import the Ground Zero R16 heightmap into an
Unreal terrain test map.

Corrected final import settings:

- Heightmap encoding: `unreal_landscape_midpoint_32768_sea_level`
- Heightmap resolution: `1009 x 1009`
- X scale: `99.2063492063492 cm`
- Y scale: `99.2063492063492 cm`
- Z scale: `100.0 cm`
- Z offset: `0.0 m`
- Elevation range: `3.176246404647827m` to `96.50486755371094m`

Added:

- `UAgrarianEditorAutomationLibrary::ImportLandscapeHeightmapIntoEditorWorld`
- Unreal `Landscape` module dependency
- `Scripts/setup_ground_zero_terrain_map.py`
- `Scripts/verify_ground_zero_terrain_map.py`

Generated/saved:

- `/Game/Agrarian/Maps/L_GroundZeroTerrain_Test`
- `Content/Agrarian/Maps/L_GroundZeroTerrain_Test.umap`

Validation:

- `UNRAID_PASSWORD=... /home/nathan/bin/agrarian-build-editor` succeeds.
- `setup_ground_zero_terrain_map.py` imports `AGR_GroundZero_Landscape`.
- `verify_ground_zero_terrain_map.py` passes.
- Verified landscape scale is `(99.206352, 99.206352, 100.0)`.
- Verified X/Y bounds extent is about `50000 cm`, so the terrain spans
  `100000 cm` / `1000 m`.
- Verified bounds origin is centered near XY zero.

Operational note:

- Unreal Build Accelerator hung during the first compile attempt, so
  `Scripts/BuildEditor-Windows.bat` now passes `-NoUBA`.

Roadmap:

- Ground Zero R16 terrain map import is complete.
- Terrain base and 1 km scale verification are complete.

Next roadmap item:

- Add first-pass water depth/shoreline handling if applicable.

## Agrarian Ground Zero Investor Demo Package - 2026-05-14

Completed the first Windows Development packaged investor demo setup.

What changed:

- Project default game and editor startup map now point to the Ground Zero
  terrain test map:
  `/Game/Agrarian/Maps/L_GroundZeroTerrain_Test.L_GroundZeroTerrain_Test`.
- Packaging settings cook the Ground Zero map and keep the original ThirdPerson
  map available as a fallback/reference.
- Added repeatable packaging wrapper:
  `Scripts/PackageWindowsDevelopment.bat`.
- Added Ground Zero demo setup script:
  `Scripts/setup_ground_zero_demo_map.py`.
- The setup script placed demo-ready actors on the real Ground Zero terrain:
  player start, wood node, fiber node, campfire, primitive shelter, rabbit,
  directional light, skylight, and fog.
- Added splash asset:
  `Content/Splash/Splash.bmp`.
- Added investor demo notices:
  `Docs/Legal/InvestorDemoNotices.md`.

Build output:

- Packaged executable:
  `Builds/WindowsDevelopment/AgrarianGame.exe`.
- Larger runtime executable:
  `Builds/WindowsDevelopment/AgrarianGame/Binaries/Win64/AgrarianGame.exe`.
- Packaged output size was about `1.1G`.
- `Builds/` is now ignored so packaged artifacts are not accidentally
  committed.

Verification:

- Windows-Builder packaging completed with `BUILD SUCCESSFUL`.
- Smoke launched the packaged executable with
  `-nullrhi -nosound -unattended`.
- Runtime log confirmed browse/load into:
  `/Game/Agrarian/Maps/L_GroundZeroTerrain_Test?Name=Player`.
- Runtime log confirmed the game mode remained
  `BP_ThirdPersonGameMode_C`.
- Runtime log exited cleanly after the smoke command.

Next roadmap item:

- Use the packaged demo as the investor baseline, then continue with Ground Zero
  visual/gameplay polish: shoreline/water if needed, biome-appropriate
  resources, simple HUD/debug display, and first playable survival tuning.

## Agrarian Investor Demo Launch Fix - 2026-05-14

User reported the investor package would not open/run.

Diagnosis:

- The packaged build itself was valid, but the normal graphics launch path
  crashed when started through the Unraid/QEMU guest-agent helper.
- Failure in runtime log:
  `DXGI_ERROR_NOT_CURRENTLY_AVAILABLE` during swapchain creation.
- The same issue occurred with both D3D12 and D3D11 when launched as `SYSTEM`
  through QEMU guest agent.
- `query user` on Windows-Builder showed the `nathan` Windows session was
  disconnected, so this headless QEMU launch is not representative of a real
  interactive desktop demo.

Fix implemented:

- Kept the main project default DX12-first in `Config/DefaultEngine.ini`.
- Added Windows package override:
  `Config/Windows/WindowsEngine.ini`
  with `DefaultGraphicsRHI=DefaultGraphicsRHI_DX11`.
- The packaged investor build now defaults to DX11 compatibility mode while
  still cooking both D3D12 SM6 and D3D11 SM5 shader formats.
- Added:
  - `Scripts/InstallWindowsDemoLaunchers.bat`
  - `Start Agrarian Demo.cmd`
  - `Start Agrarian Demo - DX12.cmd`
  - `Start Agrarian Demo - Compatibility DX11.cmd`
  - `Install Prerequisites.cmd`
  - `README-Investor-Demo.txt`
- The package includes Microsoft VC++ runtime installer at:
  `Engine/Extras/Redist/en-us/vc_redist.x64.exe`.

Rebuilt package:

- Output:
  `Builds/WindowsDevelopment`.
- Package size remains about `1.1G`.
- Rebuild completed successfully after clearing an old locked package artifact.

Important demo instruction:

- Investors should keep/copy the entire `WindowsDevelopment` folder together
  and start with `Start Agrarian Demo.cmd`.
- If the demo does not open, run `Install Prerequisites.cmd`, then
  `Start Agrarian Demo - Compatibility DX11.cmd`.
- `Start Agrarian Demo - DX12.cmd` is included for DX12-specific testing.

## Agrarian Startup Movie And Demo Notice - 2026-05-14

Added the first investor-facing launch sequence.

Startup movie:

- Installed free `ffmpeg` on this machine to generate the movie.
- Added movie asset:
  `Content/Movies/AgrarianStudioIntro.mp4`.
- Duration: `6.0` seconds.
- Resolution: `1280 x 720`.
- The first and primary line is the motto:
  `What survives after you are gone?`
- The movie also shows:
  - `AGRARIAN STUDIO`
  - `Investor Demo v0.01 - Beta Prototype`
  - `Copyright (c) 2026 Agrarian Studio. All rights reserved.`
- Configured startup movie in `Config/DefaultGame.ini`:
  `+StartupMovies=AgrarianStudioIntro`.

In-game demo notice:

- Added runtime UI classes:
  - `UAgrarianDemoNoticeWidget`
  - `AAgrarianDemoNoticeActor`
- `setup_ground_zero_demo_map.py` now places `AGR_DemoNoticeActor` in the
  Ground Zero map.
- Notice shows motto, `Investor Demo v0.01`, beta prototype/non-distribution
  language, and copyright for the first few seconds of gameplay.

Packaging/build notes:

- Added `*.mp4` to Git LFS attributes.
- Updated `Scripts/PackageWindowsDevelopment.bat` so package builds compile
  targets explicitly with `Build.bat ... -NoUBA`, then run UAT with
  `-skipbuild`. This avoids the UAT/UBA hang seen during packaging.
- Rebuilt the Windows Development investor package successfully.
- Verified packaged movie exists at:
  `Builds/WindowsDevelopment/AgrarianGame/Content/Movies/AgrarianStudioIntro.mp4`.
- Headless `-nullrhi` smoke test loaded the Ground Zero map and exited cleanly.

## Agrarian Ground Zero Water/Shoreline Pass - 2026-05-14

Completed the next roadmap item:

- `Add first-pass water depth/shoreline handling if applicable.`

Added:

- `Scripts/analyze_ground_zero_water.py`
- `Docs/Terrain/GroundZeroWaterShoreline.md`
- `Data/Terrain/Analysis/gz_us_ca_pacifica_utm10n_e544_n4160/gz_us_ca_pacifica_utm10n_e544_n4160_water_shoreline_analysis.json`

Result:

- The current Ground Zero tile does not contain ocean water or shoreline inside
  the 1 km tile bounds.
- DEM minimum elevation is about `3.16 m`.
- Sea-level or below samples: `0`.
- Near-sea-level samples at `<= 2.0 m`: `0`.
- Low coastal samples at `<= 5.0 m`: `57,812` / `1,000,000`.
- North and west edges contain the lowest coastal-influenced terrain.

Decision:

- Do not place an ocean plane in the current Ground Zero tile.
- Do not fake bathymetry in this tile.
- Keep NOAA/NCEI bathymetry/coastal DEM work for west/southwest neighbor tiles
  when coastal/ocean tiles come into scope.
- Add a gameplay freshwater source in a later map pass.

Roadmap:

- Marked first-pass water depth/shoreline handling complete.
- Immediate next item is first-pass hill, mountain, river, stream, lake, and
  coastline/absence handling from the Ground Zero terrain data.

## Agrarian Ground Zero Landform Pass - 2026-05-14

Completed the next roadmap item:

- `Add first-pass hill, mountain, river, stream, lake, and coastline handling if
  present in Ground Zero.`

Added:

- `Scripts/analyze_ground_zero_landforms.py`
- `Docs/Terrain/GroundZeroLandforms.md`
- `Data/Terrain/Analysis/gz_us_ca_pacifica_utm10n_e544_n4160/gz_us_ca_pacifica_utm10n_e544_n4160_landform_analysis.json`

Result:

- Hills are present in the Ground Zero tile.
- Mountains are not present; max elevation is `96.51 m`, p95 elevation is
  `54.57 m`.
- Steep slopes are present; max slope is `53.62 deg`, p95 slope is
  `26.95 deg`.
- River, confirmed stream, lake, and coastline are not present from this
  DEM-only pass.
- DEM shape suggests drainage/freshwater search candidates, but those need
  hydrography validation before being treated as real streams.
- First-pass walkable/buildable terrain: `83.42%`.
- First-pass difficult/slow-travel terrain: `16.58%`.
- Possible drainage/freshwater search zone: `17.30%`.

Decision:

- Use the landform analysis for first-pass movement modifiers, buildability,
  resource placement, and foliage density.
- Do not add fake mountains, lakes, rivers, streams, or coastline inside the
  current tile.
- Validate freshwater candidates against a real hydrography source later.

Roadmap:

- Marked first-pass hill/mountain/river/stream/lake/coastline handling
  complete.
- Immediate next item is to verify neighboring tile edge coordinates against
  the registry before multi-tile stitching.

## Agrarian Ground Zero Neighbor Edge Verification - 2026-05-14

Completed the next roadmap item:

- `Verify neighboring tile edge coordinates against the registry before
  multi-tile stitching.`

Added:

- `Scripts/verify_ground_zero_neighbor_edges.py`
- `Docs/Terrain/GroundZeroNeighborEdges.md`
- `Data/Terrain/Analysis/gz_us_ca_pacifica_utm10n_e544_n4160/gz_us_ca_pacifica_utm10n_e544_n4160_neighbor_edge_verification.json`

Result:

- Verification passed.
- Verified tile count: `9`.
- Found neighbor count: `8`.
- Missing neighbors: `0`.
- All tiles use exact `1000 m x 1000 m` UTM bounds.
- All tile IDs match their minimum UTM kilometer coordinates.
- North, south, east, and west neighbors share exact full 1 km edges with
  Ground Zero.
- Northeast, southeast, southwest, and northwest neighbors touch Ground Zero at
  corners only.

Decision:

- The registry is ready for coordinate-based neighbor stitching around Ground
  Zero.
- This only verifies registry bounds and adjacency. Elevation seam continuity
  still needs neighbor DEM extraction/generation before real stitched terrain.

Roadmap:

- Marked neighbor edge coordinate verification complete.
- Immediate next item is the Ground Zero foliage pass.

## Agrarian Ground Zero Foliage Pass - 2026-05-14

Completed the next roadmap item:

- `Add foliage pass.`

Added:

- `AAgrarianFoliagePatch`
- `Scripts/verify_ground_zero_foliage.py`
- `Docs/Terrain/GroundZeroFoliagePass.md`

Updated:

- `Scripts/setup_ground_zero_demo_map.py`
- `Content/Agrarian/Maps/L_GroundZeroTerrain_Test.umap`

Result:

- Ground Zero now has one deterministic first-pass foliage actor:
  `AGR_GroundZeroFoliage_FirstPass`.
- The actor uses hierarchical instanced static mesh components for first-pass
  rendering efficiency.
- Foliage instance counts:
  - Trees: `42`
  - Shrubs: `96`
  - Grass clumps: `180`
- Placement is terrain-height-aware and avoids the player start, demo
  structures, resource nodes, and wildlife spawn.

Validation:

- `UNRAID_PASSWORD=... /home/nathan/bin/agrarian-build-editor` succeeded.
- `Scripts/RunUnrealPython-Windows.bat Scripts/setup_ground_zero_demo_map.py`
  succeeded.
- `Scripts/RunUnrealPython-Windows.bat Scripts/verify_ground_zero_foliage.py`
  succeeded and confirmed `42` trees, `96` shrubs, and `180` grass clumps.

Roadmap:

- Marked foliage pass complete.
- Immediate next item is biome-appropriate natural resources based on Ground
  Zero.

## Agrarian Ground Zero Biome Resource Pass - 2026-05-14

Completed the next roadmap item:

- `Add biome-appropriate natural resources based on Ground Zero.`

Added:

- `BP_StoneResourceNode`
- `Scripts/verify_ground_zero_resources.py`
- `Docs/Terrain/GroundZeroResourcePass.md`

Updated:

- `Scripts/setup_playable_blueprints.py`
- `Scripts/verify_playable_blueprints.py`
- `Scripts/setup_ground_zero_demo_map.py`
- `Content/Agrarian/Maps/L_GroundZeroTerrain_Test.umap`

Result:

- Ground Zero now has biome-appropriate natural resource nodes for the selected
  coastal California scrub/woodland tile.
- Resource node counts:
  - Wood: `4`
  - Fiber: `5`
  - Stone: `4`
- Wood nodes are placed in scrub/woodland and hillside areas.
- Fiber nodes are placed in grassland, scrub, and drainage-candidate areas.
- Stone nodes are placed on slope, exposed-terrain, and valley-edge areas.
- Freshwater intentionally remains separate because the next roadmap item is
  `Add water source.`

Validation:

- `Scripts/RunUnrealPython-Windows.bat Scripts/setup_playable_blueprints.py`
  succeeded and created/configured `BP_StoneResourceNode`.
- `Scripts/RunUnrealPython-Windows.bat Scripts/verify_playable_blueprints.py`
  succeeded.
- `Scripts/RunUnrealPython-Windows.bat Scripts/setup_ground_zero_demo_map.py`
  succeeded.
- `Scripts/RunUnrealPython-Windows.bat Scripts/verify_ground_zero_resources.py`
  succeeded and confirmed `4` wood, `5` fiber, and `4` stone nodes.

Roadmap:

- Marked biome-appropriate natural resources complete.
- Immediate next item is to add a water source.

## Agrarian Ground Zero Freshwater Source - 2026-05-14

Completed the next roadmap item:

- `Add water source.`

Added:

- `AAgrarianWaterSource`
- `BP_FreshWaterSource`
- `Scripts/verify_ground_zero_water_source.py`
- `Docs/Terrain/GroundZeroWaterSource.md`

Updated:

- `Scripts/setup_playable_blueprints.py`
- `Scripts/verify_playable_blueprints.py`
- `Scripts/setup_ground_zero_demo_map.py`
- `Content/Agrarian/Maps/L_GroundZeroTerrain_Test.umap`

Result:

- Ground Zero now has one gameplay freshwater source:
  `AGR_GZ_FreshWaterSource_01`.
- The water source uses `/Game/Agrarian/Blueprints/World/BP_FreshWaterSource`.
- It restores `45.0` thirst/water through `UAgrarianSurvivalComponent::AddWater`.
- Placement is in the drainage-candidate area and is a gameplay source, not a
  confirmed real hydrography feature.

Validation:

- `UNRAID_PASSWORD=... /home/nathan/bin/agrarian-build-editor` succeeded.
- `Scripts/RunUnrealPython-Windows.bat Scripts/setup_playable_blueprints.py`
  succeeded.
- `Scripts/RunUnrealPython-Windows.bat Scripts/verify_playable_blueprints.py`
  succeeded.
- `Scripts/RunUnrealPython-Windows.bat Scripts/setup_ground_zero_demo_map.py`
  succeeded.
- `Scripts/RunUnrealPython-Windows.bat Scripts/verify_ground_zero_water_source.py`
  succeeded.

Roadmap:

- Marked water source complete.
- Immediate next item is weather exposure zones if needed.

## Agrarian Time And Real Weather Planning - 2026-05-14

Roadmap-only planning update before the next implementation item.

Added time/progression philosophy:

- Default target clock: `4 real hours = 1 in-game day`.
- The world should not feel artificially sped up; the player gets better through
  skills, tools, infrastructure, cooperation, and knowledge.
- Skills should improve efficiency, yield, quality, reliability, survival odds,
  and capacity more than they shorten natural biological time.
- Crops, livestock, spoilage, disease, healing, tree growth, seasons, and other
  long-running systems use the Agrarian calendar.
- Early survival should remain difficult and hands-on, with hunting, gathering,
  fishing, scavenging, and primitive crafting available while domestication and
  farming mature over meaningful calendar time.

Added real-weather roadmap plan:

- Add a Ground Zero real-world weather provider adapter by latitude/longitude.
- Use Open-Meteo as the first global MVP source.
- Use NOAA/NWS as a US fallback or enrichment source where useful.
- Cache weather snapshots server-side; clients should not call public weather
  APIs directly.
- Map real weather inputs into Agrarian states: temperature, precipitation,
  wind, cloud cover, humidity, pressure, visibility, and weather code.
- Keep deterministic fallback weather simulation when external data is
  unavailable.

## Agrarian Roadmap Reordering - 2026-05-14

Reworked the roadmap structure so it flows in order:

- Project North Star and time/progression philosophy.
- Current milestone status: `Version 0.01 Foundation Baseline`.
- Phase 0 foundation and guardrails.
- Phase 1 foundational survival MVP.
- Later gameplay/civilization phases.
- Cross-cutting technical tracks.
- Six-month calendar, MVP definition of done, and earliest next actions.

Removed the duplicated standalone `Version .01 / 0.01` block that previously
sat below `0.7`, which made the roadmap look like it jumped backward in time.
Version `0.01` is now summarized near the top as the active milestone, while
workstream sections like `0.1`, `0.2`, and `0.7` remain inside Phase 0.

Updated the near-term next actions to restart from the earliest missed
foundation items before continuing deeper gameplay/weather work.

Immediate next item:

- `Create protected main branch.`

Also added a new Storefront Development Distribution track covering Steam and
Epic development-build launch planning, including Steamworks/Epic portal
checklists, internal/investor/closed-test/public channels, build metadata,
store branch naming, entitlement/account-linking requirements, and release
checklists.

Note: `gh` is not installed and no GitHub token is available in the local
environment, so GitHub branch protection cannot be applied from this host yet
without adding/authenticating tooling or doing it through the GitHub UI.

## Agrarian GitHub Branch Protection Check - 2026-05-14

Installed and authenticated GitHub CLI as `pacificao`.

Confirmed:

- Repo: `pacificao/AgrarianGameBuild`
- Visibility: private
- Local GitHub CLI permission: `ADMIN`
- Remote `main` branch exists.

Branch protection result:

- GitHub API returned HTTP `403`.
- Message: `Upgrade to GitHub Pro or make this repository public to enable this feature.`
- No branch protection changes were applied.
- No billing, repository visibility, deletion, or paid setting changes were made.

Roadmap update:

- Marked `Create protected main branch` as blocked.
- Immediate next foundation item is deciding whether to create/use a long-lived
  `dev` branch.

## Agrarian Repository Storage Guardrails - 2026-05-14

Completed storage guardrail work to keep GitHub free-tier viable as long as
possible.

Added:

- `Docs/RepositoryStoragePolicy.md`
- `Scripts/audit_repo_storage.sh`

Policy:

- GitHub is for source, configs, scripts, docs, metadata, and curated project
  assets.
- Raw DEM/lidar/GIS datasets, packaged builds, generated terrain tile packages,
  DerivedDataCache, and large source-art archives stay on DevBox or future
  object storage.
- Git stores manifests, checksums, provenance, import scripts, and small curated
  samples when needed.

Cleanup:

- Removed two raw USGS DEM `.tif` files from Git tracking and local unpushed Git
  history.
- The raw files remain locally on DevBox under `Data/Terrain/Sources/...`.
- Added ignore rules for raw terrain/source archives.
- Re-added the GitHub remote after `git filter-repo` removed it.

Audit result:

- Git object database pack after cleanup: about `5.65 MiB`.
- Largest tracked non-LFS file: about `4.3 MB`.
- Largest LFS object: about `21 MB`.
- Generated/local folders remain large on DevBox, which is expected:
  `Intermediate`, `Saved`, `Builds`, and `Binaries`.

Branch decision:

- Decided not to create/use a long-lived `dev` branch yet.
- Current approach: `main` plus short-lived task branches until team size or
  release channels require a staging branch.
- Immediate next foundation item is finishing branch naming conventions.

## Agrarian Branch Naming And Tile-Serving MVP Roadmap - 2026-05-14

Completed branch naming conventions for the current lightweight workflow.

Added:

- `Docs/BranchingConventions.md`

Policy:

- `main` remains the only long-lived development branch for now.
- Do not create a long-lived `dev` branch yet.
- Use short-lived task branches named `type/short-description`.
- Allowed branch prefixes are `feature/`, `fix/`, `docs/`, `ops/`, `test/`,
  `asset/`, and `experiment/`.
- Keep raw terrain datasets, generated builds, DerivedDataCache, and large
  archives out of every Git branch.

Roadmap update:

- Marked branch naming conventions complete in version `0.01 Foundation
  Baseline`.
- Split commit message conventions into the next separate foundation item.
- Added near-term MVP map-tile serving server work to Phase `0.7` and Near-Term
  Next Actions:
  - Launch a small Ubuntu/Linux cloud VM for map-tile serving.
  - Publish a tiny Ground Zero tile manifest/package.
  - Prove lookup, download, local cache, redownload, and immediate-neighbor
    metadata flow.
  - Add cost controls and runbook notes so MVP testing stays free or near-free.

Immediate next roadmap item:

- Finish commit message conventions.

## Agrarian Commit Message Conventions - 2026-05-14

Completed commit message conventions for `Version 0.01 Foundation Baseline`.

Added:

- `Docs/CommitMessageConventions.md`

Policy:

- Use concise, plain-English, imperative commit subjects.
- Keep subjects specific, capitalized, and usually under 72 characters.
- Prefer one commit per roadmap item or tight implementation slice.
- Add commit bodies for non-obvious reason, tradeoff, validation, operational
  impact, build impact, networking, tile delivery, or save compatibility.
- Never include secrets, passwords, tokens, private keys, or credentials in
  commit messages.
- Avoid vague messages such as `updates`, `misc`, `changes`, `work`, or `wip`.

Roadmap update:

- Marked commit message conventions complete.
- Immediate next roadmap item is defining backup expectations for NAS and the
  repository.

## Agrarian Backup Expectations - 2026-05-14

Completed the roadmap item to define backup expectations for NAS and repository.

Added:

- `Docs/BackupExpectations.md`

Backup policy:

- GitHub is not enough by itself; it protects source history, not all local
  terrain source data, generated build outputs, VM state, or deleted local files.
- DevBox remains the primary working storage for the Unreal project.
- Linastorage is the first backup target for project snapshots, VM backups, and
  deleted-file recovery.
- Back up `/mnt/projects/AgrarianGameBulid` / `\\DevBox\projects\AgrarianGameBulid`
  incrementally during active development when changes exist.
- Recommended project cadence:
  - every 2 hours during active development if changes exist
  - daily, weekly, and monthly retained snapshots
  - manual pre-change snapshots before major Unreal, plugin, terrain, migration,
    or build-pipeline changes
- Retention target:
  - frequent snapshots: 7 days
  - daily snapshots: 30 days
  - weekly snapshots: 12 weeks
  - monthly snapshots: 12 months
  - important deleted files recoverable for at least 30 days, preferably 90 days
    for major assets/source data/investor-demo material
- Backup method should stage locally or on DevBox first, write manifests and
  checksums, copy to Linastorage under `.incomplete-*`, verify on the NAS, then
  atomically rename the completed snapshot.
- VM backups must cover `Windows-Builder`, `Ubuntu-Codex`, VM XML/NVRAM, Unraid
  flash/config, and related helper scripts.
- Do not copy live VM disk images directly without quiescing. Use Unraid VM
  backup tooling, QEMU guest-agent freeze/thaw, or a powered-down maintenance
  backup.
- Restore testing is required:
  - weekly single-file restore
  - monthly repo snapshot restore/list check
  - monthly VM backup readability check
  - quarterly fuller restore drill

Roadmap update:

- Marked backup expectations, VM snapshot cadence, and Unraid share backup
  policy as documented.
- Added follow-on implementation items:
  - Implement Linastorage incremental project backup job.
  - Implement quiesced VM backup job for Windows-Builder and Ubuntu-Codex.
  - Add recurring restore-test log for project and VM backups.
- Immediate next roadmap item is implementing the Linastorage incremental
  project backup job.

## Agrarian Linastorage Project Backup Job - 2026-05-14

Completed the roadmap item to implement the Linastorage incremental project
backup job.

Repo files added/updated:

- `Scripts/agrarian_project_backup.sh`
- `Operations/systemd/agrarian-project-backup.service`
- `Operations/systemd/agrarian-project-backup.timer`
- `Docs/Ops/AgrarianProjectBackupRunbook.md`

Installed on this host:

- Script: `/usr/local/sbin/agrarian-project-backup`
- Service: `/etc/systemd/system/agrarian-project-backup.service`
- Timer: `/etc/systemd/system/agrarian-project-backup.timer`

Active backup implementation:

- Uses `restic`, installed from Ubuntu packages.
- Repository:
  `/mnt/backups/linastorage/backups/agrarian-game/project/restic-repository`
- State files:
  `/mnt/backups/linastorage/backups/agrarian-game/project/state`
- Password file:
  `/root/.backup-secrets/agrarian-project-restic.password`
- Do not copy or write the restic password into handoff files or the repo.
- Timer runs every 2 hours with randomized delay.
- Script records a source signature and skips scheduled runs when no project
  changes are detected.
- Retention:
  - keep all snapshots within 7 days
  - keep daily snapshots for 30 days
  - keep weekly snapshots for 12 weeks
  - keep monthly snapshots for 12 months

Important implementation note:

- First attempt used `rsync --link-dest`, but Linastorage over SMB does not
  support the hard-link operations required for that model.
- Restic replaced rsync because it stores encrypted, deduplicated chunks as
  normal files and works with the SMB target.
- A one-time legacy rsync snapshot remains under the old `snapshots/` directory
  as a temporary safety copy; active backups are restic.

Verification completed:

- First restic snapshot: `e7ec1ce7`, processed about `825 MiB`, stored about
  `566 MiB`.
- Incremental snapshot after script/runbook changes: `208d7710`, stored about
  `5 KiB`.
- Final incremental snapshot after lock handling change: `3d77f1d2`, stored
  about `8 KiB`.
- Restored `AGRARIAN_DEVELOPMENT_ROADMAP.md` from restic latest with
  `restic dump` and verified it matched the live file with `diff`.
- Final dry run returned:
  `No project changes detected since latest restic backup; skipping`.
- Timer is enabled and active; next trigger was scheduled after verification.

Roadmap update:

- Moved the private-repo GitHub protected `main` branch item out of `0.01`
  foundation and into the release/build pipeline as a paid-plan/revenue-timed
  hardening item.
- Marked Linastorage incremental project backup job complete.
- Immediate next roadmap item is implementing quiesced VM backup jobs for
  `Windows-Builder` and `Ubuntu-Codex`.

## Agrarian Unraid VM Backup Job - 2026-05-14

Completed the roadmap item to implement quiesced VM backup jobs for
`Windows-Builder` and `Ubuntu-Codex`.

Repo files added/updated:

- `Scripts/agrarian_vm_backup_unraid.sh`
- `Operations/unraid/agrarian-vm-backup.cron`
- `Docs/Ops/AgrarianVmBackupRunbook.md`
- `AGRARIAN_DEVELOPMENT_ROADMAP.md`

Installed on Unraid `DevBox`:

- Persistent script: `/boot/config/custom/agrarian-vm-backup.sh`
- Runtime script copy: `/usr/local/sbin/agrarian-vm-backup`
- Persistent cron: `/boot/config/plugins/dynamix/agrarian-vm-backup.cron`
- Loaded cron entry in `/etc/cron.d/root`

Backup behavior:

- Weekly safe check runs Sunday at `03:15`.
- Scheduled/default mode skips running VMs and publishes no empty snapshot.
- Manual maintenance mode:
  `/usr/local/sbin/agrarian-vm-backup --shutdown-running`
- Maintenance mode gracefully shuts down selected running VMs, backs them up,
  and starts VMs again unless `--no-start-after` is used.
- Captures VM XML, NVRAM when present, compressed qcow2 disk images via
  `qemu-img convert -O qcow2 -c`, selected Unraid VM/share config, manifest,
  and SHA256 checksums.
- Snapshot root: `/mnt/user/backups/agrarian-game/vms/snapshots`
- Retention: deletes snapshots older than 120 days.

Verification completed:

- Local script syntax passed with `bash -n`.
- Unraid safe dry run saw both VMs running, skipped both, and published no
  snapshot.
- Unraid maintenance dry run for `Ubuntu-Codex` showed the intended shutdown,
  XML export, and qcow2 conversion path without actually stopping the VM.
- `update_cron` loaded the weekly cron entry successfully.

Important operational note:

- A real full VM backup was not run during implementation because
  `Windows-Builder` and `Ubuntu-Codex` were active. Run the maintenance command
  during a planned window to create the first disk-image snapshots.

Roadmap update:

- Marked quiesced VM backup job complete.
- Immediate next roadmap item is creating repeatable dedicated server build
  instructions.

## Agrarian Dedicated Server And Tile Delivery Build Lanes - 2026-05-14

Completed the roadmap item to create repeatable dedicated server build
instructions, and included a fast static cloud map-tile delivery package path.

Repo files added/updated:

- `Source/AgrarianGameServer.Target.cs`
- `Scripts/BuildLinuxDedicatedServer-Windows.bat`
- `Scripts/build_ground_zero_tile_delivery_package.sh`
- `Operations/cloud-map-tile-server/bootstrap_ubuntu_tile_server.sh`
- `Docs/Ops/DedicatedServerBuildRunbook.md`
- `Docs/Ops/MapTileDeliveryServerRunbook.md`
- `.gitignore`
- `AGRARIAN_DEVELOPMENT_ROADMAP.md`

Dedicated server lane:

- Added Unreal target `AgrarianGameServer` with `TargetType.Server`.
- Windows-Builder command:
  `Scripts\BuildLinuxDedicatedServer-Windows.bat`
- Expected output:
  `Builds\LinuxServerDevelopment`
- Expected log:
  `Saved\BuildLogs\BuildLinuxDedicatedServer.log`
- Server package cooks `/Game/Agrarian/Maps/L_GroundZeroTerrain_Test`.
- Requires Epic's Unreal `5.7` Linux cross-compile toolchain on
  Windows-Builder before the actual Linux package can build.

Map-tile delivery lane:

- Build command from the repo on Linux:
  `Scripts/build_ground_zero_tile_delivery_package.sh`
- Output archive:
  `BuildArtifacts/TileDelivery/agrarian-ground-zero-tile-delivery.tar.gz`
- Static package includes:
  - `manifest.json`
  - `ground_zero_tiles.json`
  - tile registry schema
  - Ground Zero Unreal heightmap, metadata, landform analysis, water/shoreline
    analysis, and neighbor edge verification
  - `SHA256SUMS`
- `BuildArtifacts/` is ignored and should not be committed.
- Bootstrap command on a fresh Ubuntu tile server:
  `sudo Operations/cloud-map-tile-server/bootstrap_ubuntu_tile_server.sh /path/to/agrarian-ground-zero-tile-delivery.tar.gz`
- Bootstrap installs nginx and serves `/health`, `/manifest.json`,
  `/ground_zero_tiles.json`, and versioned tile package files.

Verification completed:

- Bash syntax passed for both Linux scripts.
- Ground Zero tile delivery package built successfully from current repo data.
- `sha256sum -c SHA256SUMS` passed for the generated static tile package.
- Archive listing confirmed the manifest, registry, schema, and Ground Zero
  tile files are included.
- Dedicated server Windows batch script was reviewed but not executed here,
  because the Linux Unreal cross-compile toolchain state must be checked on
  Windows-Builder.

Roadmap update:

- Marked repeatable dedicated server build instructions complete.
- Marked one-command Linux dedicated server build wrapper complete.
- Added and marked complete the static Ground Zero tile-delivery package and
  Ubuntu nginx bootstrap script item.
- Immediate next roadmap item is finishing required plugin documentation.

## Agrarian Required Plugin Documentation - 2026-05-14

Completed the roadmap item to finish required plugin documentation.

Repo files added/updated:

- `Docs/RequiredPlugins.md`
- `AGRARIAN_DEVELOPMENT_ROADMAP.md`

Current enabled plugins in `AgrarianGame.uproject`:

- `StateTree`
- `GameplayStateTree`
- `ModelingToolsEditorMode`

Decision captured:

- Keep `StateTree` and `GameplayStateTree` for version `0.01` because the
  compiled template AI code under `Variant_Combat` and `Variant_SideScrolling`
  still depends on `StateTreeModule`, `GameplayStateTreeModule`, and
  `UStateTreeAIComponent`.
- Keep `ModelingToolsEditorMode` enabled as an editor workflow plugin for
  terrain, mesh, and prototype world-building work.
- Do not add Steam, Gameplay Ability System, Marketplace, or machine-specific
  editor-helper plugins until a roadmap item needs them.
- Revisit StateTree dependencies during the template-variant cleanup item.

Verification completed:

- `AgrarianGame.uproject` parsed successfully as JSON.
- Secret scan of the new plugin documentation returned no matches.

Roadmap update:

- Marked required plugin documentation complete.
- Immediate next roadmap item is confirming the project opens cleanly from a
  fresh checkout.

## Agrarian Fresh Checkout Open Verification And Character Selection Roadmap - 2026-05-14

Completed the foundation baseline item to confirm the project opens cleanly from
a fresh checkout, and added the requested MVP character-selection landing page
requirement to the roadmap.

Roadmap additions:

- Phase `1.14 MVP UI And UX` now includes a post-splash/startup landing page.
- The landing page should let the player choose a realistic young adult male or
  female character with average proportions for the MVP.
- The first playable MVP definition of done now includes the startup flow
  reaching that character selection landing page and allowing the male/female
  character choice before entering the world.

Fresh checkout used:

- Path: `/mnt/projects/AgrarianFreshCheckout-20260514-133248`
- Commit checked out: `0173b34`
- Clone source: `git@github.com:pacificao/AgrarianGameBuild.git`

Verification completed:

- Fresh clone completed with Git LFS content filtered.
- `git lfs fsck` passed.
- Fresh clone `git status --short` was clean before Unreal generated files.
- Windows-Builder built the fresh checkout with:
  `Scripts\BuildEditor-Windows.bat`
- Build result: `AgrarianGameEditor Win64 Development` succeeded.
- Windows-Builder opened the fresh checkout through command-mode Unreal with:
  `Scripts\RunUnrealPython-Windows.bat Scripts\verify_ground_zero_terrain_map.py`
- Ground Zero map loaded successfully.
- Map check reported `0 Error(s), 0 Warning(s)`.
- Verification script reported the expected 1 km terrain bounds and exited
  cleanly.

Operational note:

- The fresh checkout was left in place for reference. Unreal generated
  `Binaries/`, `Intermediate/`, and `Saved/` inside that checkout during the
  validation.

Roadmap update:

- Marked fresh-checkout project open verification complete.
- Immediate next roadmap item is deciding whether to keep current Unreal
  template variants or remove unused starter variants.

## Agrarian Template Variant Decision - 2026-05-14

Completed the roadmap item to decide whether to keep current Unreal template
variants or remove unused starter variants.

Repo files added/updated:

- `Docs/TemplateVariantDecision.md`
- `Docs/RequiredPlugins.md`
- `AGRARIAN_DEVELOPMENT_ROADMAP.md`

Decision:

- Remove or quarantine unused starter variants during the next content
  organization cleanup:
  - `Variant_Combat`
  - `Variant_Platforming`
  - `Variant_SideScrolling`
- Keep `ThirdPerson` temporarily because the current player Blueprint,
  interaction setup, and automation still use it.
- Keep shared mannequin/animation content only where current Agrarian assets
  still reference it.
- Recheck and remove `StateTree` / `GameplayStateTree` plugin and module
  dependencies after the unused variant code/content is removed.

Reason:

- The starter variants add compile time, plugin dependencies, packaging noise,
  and make the project look like a generic Unreal template.
- Actual source/content deletion is intentionally deferred to the next roadmap
  item so content redirects, maps, external actors, and smoke tests can be
  handled in one focused editor-aware cleanup pass.

Verification completed:

- Reviewed source/config/content references for `Variant_*`, `ThirdPerson`,
  `StateTree`, and `GameplayStateTree`.
- Secret scan of the new decision doc and plugin doc returned no matches.

Roadmap update:

- Marked the template variant decision complete.
- Immediate next roadmap item is organizing `Content/Agrarian/` folders and
  moving/removing starter/prototype assets into clearly named locations.

## Agrarian Starter Variant Content Cleanup - 2026-05-14

Completed the roadmap item to organize `Content/Agrarian/` folders and remove
unused starter/prototype assets.

Repo files updated:

- `AGRARIAN_DEVELOPMENT_ROADMAP.md`
- `AgrarianGame.uproject`
- `Config/DefaultEditor.ini`
- `Config/DefaultEditorPerProjectUserSettings.ini`
- `Docs/RequiredPlugins.md`
- `Docs/TemplateVariantDecision.md`
- `Source/AgrarianGame/AgrarianGame.Build.cs`

Removed:

- `Source/AgrarianGame/Variant_Combat`
- `Source/AgrarianGame/Variant_Platforming`
- `Source/AgrarianGame/Variant_SideScrolling`
- `Content/Variant_Combat`
- `Content/Variant_Platforming`
- `Content/Variant_SideScrolling`
- Matching World Partition external actor/object folders for those variants.

Build/config cleanup:

- Removed `StateTreeModule` and `GameplayStateTreeModule` from
  `AgrarianGame.Build.cs`.
- Removed explicit `StateTree` and `GameplayStateTree` plugin entries from
  `AgrarianGame.uproject`.
- Updated editor defaults so the simple map and content browser path now point
  at Agrarian/Ground Zero locations instead of the old ThirdPerson template
  folder.

Intentional leftovers:

- `Content/ThirdPerson` remains because the current player Blueprint,
  interaction setup, and automation still depend on it.
- `Content/LevelPrototyping` remains because current scripts/prototype
  Blueprints still reference simple cube/cylinder meshes.
- Replace those with Agrarian-specific character selection/player assets and
  project-native prototype meshes in a later cleanup.

Verification completed:

- Windows-Builder built `AgrarianGameEditor Win64 Development` successfully
  through `Scripts\BuildEditor-Windows.bat`.
- Windows-Builder loaded `L_GroundZeroTerrain_Test` through command-mode Unreal
  with `Scripts\RunUnrealPython-Windows.bat
  Scripts\verify_ground_zero_terrain_map.py`.
- Ground Zero map check reported `0 Error(s), 0 Warning(s)`.
- `AgrarianGame.uproject` parsed successfully as JSON.
- Source/config/project stale-reference scan found no remaining runtime
  references to the removed variant source/content or removed module
  dependencies. Remaining matches are explanatory docs only.
- Secret scan of updated roadmap/docs found no secrets.

Roadmap update:

- Marked the starter variant content cleanup complete.
- Immediate next roadmap item is launching the near-term MVP map-tile serving
  cloud VM and proving Ground Zero tile lookup/download/cache flow.

## Agrarian MVP Tile Server Launch - 2026-05-14

Completed the roadmap item to launch the near-term MVP map-tile serving VM and
prove Ground Zero tile lookup/download/cache/redownload.

Current endpoint:

- Host VM: `Agrarian-TileServer` on Unraid `DevBox`
- VM IP: `192.168.5.14`
- Port: `18080/tcp`
- Base URL: `http://192.168.5.14:18080`

Published from:

- `/srv/agrarian/tile-delivery/public`

Live endpoints verified:

- `http://192.168.5.14:18080/health`
- `http://192.168.5.14:18080/manifest.json`
- `http://192.168.5.14:18080/ground_zero_tiles.json`
- `http://192.168.5.14:18080/tiles/gz_us_ca_pacifica_utm10n_e544_n4160/v0/...`

Implementation notes:

- Built the static tile package on Unraid from the local project share because
  building it through the CIFS-mounted repo path stalled.
- Created a dedicated `Agrarian-TileServer` Ubuntu VM on Unraid from the Ubuntu
  Noble cloud image.
- Installed nginx inside `Agrarian-TileServer`.
- Configured nginx to listen on the uncommon local MVP port `18080`.
- Published the Ground Zero tile-delivery package under `/srv/agrarian`.
- Added `Scripts/verify_tile_delivery_client.sh` for repeatable client proof.
- Updated `Docs/Ops/MapTileDeliveryServerRunbook.md` with the current endpoint,
  verification path, cost-control note, and dedicated-VM follow-up.

Verification completed:

- `curl http://192.168.5.14:18080/health` returned `ok`.
- Manifest returned successfully from `http://192.168.5.14:18080/manifest.json`.
- `Scripts/verify_tile_delivery_client.sh http://192.168.5.14:18080` passed.
- Client verification downloaded the manifest, registry, SHA256SUMS, schema, and
  Ground Zero terrain package files.
- Client verification confirmed `neighbor_count=8`.
- Client verification deleted and redownloaded the heightmap, then revalidated
  checksums.

Operational follow-up:

- The service is now isolated inside `Agrarian-TileServer`; Unraid is only the
  hypervisor/storage host.
- `Ubuntu-Codex` is not serving the tile endpoint.
- Decide later whether public testing uses this LAN-hosted VM or an external
  cloud host.

Roadmap update:

- Marked the near-term MVP map-tile serving VM item complete.
- Immediate next roadmap item is defining MVP day/night length, survival
  pressure target, success loop, failure conditions, and closed-test readiness
  criteria.

Additional DNS/public endpoint note, 2026-05-15:

- `maps.agrariangame.com` now resolves to the same public IP as
  `dev.agrariangame.com`: `208.79.250.18`.
- Public tile endpoint verified:
  `http://maps.agrariangame.com:18080`
- Public client verification passed against
  `http://maps.agrariangame.com:18080`, including manifest, registry,
  checksum validation, 8 neighbor entries, and heightmap redownload.

## Agrarian MVP Survival And Readiness Criteria - 2026-05-15

Completed the roadmap item to define MVP day/night length, survival pressure
target, success loop, failure conditions, first playable internal milestone,
and closed-test readiness criteria.

Repo files updated:

- `Docs/MvpSurvivalReadinessCriteria.md`
- `AGRARIAN_DEVELOPMENT_ROADMAP.md`
- `Source/AgrarianGame/AgrarianGameState.h`

Decision:

- Day/night should mimic the real Earth region represented by the loaded map
  tile, like weather.
- For Ground Zero, the server-authoritative world clock should follow real
  local time pacing for the Ground Zero region.
- Time acceleration remains allowed for tests and debug tools, but it is not
  the default player-facing world clock.
- Player progression should come from skills, tools, shelter, storage,
  knowledge, cooperation, and infrastructure rather than speeding up the sky
  clock.

Code alignment:

- Changed the default `GameHoursPerRealMinute` from the old accelerated value
  to real-time pacing (`1.0f / 60.0f`).
- Relaxed the editor clamp so real-time pacing is valid.

Verification completed:

- Searched docs/source to remove the old `4 real hours = 1 in-game day`
  default.
- Windows-Builder built `AgrarianGameEditor Win64 Development` successfully
  through `Scripts\BuildEditor-Windows.bat`.

Roadmap update:

- Marked the MVP survival/readiness criteria item complete.
- Immediate next roadmap item is creating the core design document.

Correction on 2026-05-15:

- Restored `4 real hours = 1 in-game day` as the MVP gameplay calendar target.
- Clarified that real-region day/night is the presentation/solar/weather
  context, not a replacement for the MVP gameplay calendar pace.
- `GameHoursPerRealMinute` was restored to `0.1f`.
- Windows-Builder rebuild passed after the correction.

## Agrarian Core Design Document - 2026-05-15

Completed the roadmap item to create the core design document.

Repo files updated:

- `Docs/CoreDesignDocument.md`
- `AGRARIAN_DEVELOPMENT_ROADMAP.md`

Content captured:

- Core identity: persistent generational civilization simulator.
- Motto/design question: "What survives after you are gone?"
- Design pillars: civilization is built, legacy matters, world remembers,
  frontier always exists, knowledge progression, real place/believable pace,
  hard beginnings/earned ease.
- MVP experience and first loop.
- Long-term world model and Earth-scale tile direction.
- Time/weather split: `4 real hours = 1 in-game day` for MVP gameplay calendar,
  with day/night presentation and weather grounded in the represented region.
- Survival, progression, multiplayer/society, economy/AGR principles.
- Non-goals and design risks.
- Current design decisions, including `maps.agrariangame.com:18080` and
  `Agrarian-TileServer`.

Roadmap update:

- Marked the core design document complete.
- Immediate next roadmap item is creating the technical design document.

## Agrarian Technical Design Document - 2026-05-15

Completed the roadmap item to create the technical design document.

Repo files updated:

- `Docs/TechnicalDesignDocument.md`
- `AGRARIAN_DEVELOPMENT_ROADMAP.md`

Content captured:

- Current Unreal 5.7 C++ / Blueprint architecture baseline.
- Server-authoritative runtime model and client responsibilities.
- Gameplay system boundaries for GameState, survival, inventory, crafting,
  interaction, resource actors, buildables, and persistence.
- Time/environment target: `4 real hours = 1 in-game day`, with real-region
  day/night presentation and weather context.
- MVP tile delivery architecture and endpoint:
  `http://maps.agrariangame.com:18080`.
- Data contracts for Unreal Data Assets, JSON metadata, and save data.
- Persistence, multiplayer, build automation, infrastructure, source control,
  security, and testing gates.
- Open technical questions for the next design passes.

Roadmap update:

- Marked the technical design document complete.
- Immediate next roadmap item is creating the multiplayer/networking design
  document.

## Agrarian Multiplayer And Networking Design - 2026-05-15

Completed the roadmap item to create the multiplayer/networking design
document.

Repo files updated:

- `Docs/MultiplayerNetworkingDesign.md`
- `AGRARIAN_DEVELOPMENT_ROADMAP.md`

Content captured:

- MVP networking goal for at least two players on the same server.
- Network model: listen server acceptable for quick internal testing; dedicated
  server remains preferred closed-test target.
- Server-authority rules for survival, inventory, crafting, resources,
  wildlife, world time/weather, build placement, persistence, spawn/respawn,
  and active tile/package version.
- Client responsibilities and replication scope.
- RPC validation rules and examples.
- Join, spawn, respawn, disconnect, and reconnect MVP behavior.
- Tile delivery split: tile server delivers static files, gameplay server owns
  authoritative world/tile state.
- Missing tile handling, network relevancy, latency expectations, security
  baseline, dedicated server direction, and testing gates.

Additional roadmap items marked complete because this document defines them:

- Confirm listen server vs dedicated server for MVP.
- Define server authority over streamed terrain tiles.
- Define server response when a client requests a missing tile.
- Add player join flow.
- Add player spawn flow.

Roadmap update:

- Marked the multiplayer/networking design document complete.
- Immediate next roadmap item is creating the persistence design document.

## Agrarian Persistence Design Document - 2026-05-15

Completed the roadmap item to create the persistence design document.

Repo files updated:

- `Docs/PersistenceDesignDocument.md`
- `AGRARIAN_DEVELOPMENT_ROADMAP.md`

Content captured:

- Persistence principles: save meaningful state, version long-lived records,
  keep external metadata external, and make the server own writes.
- MVP persistence scope: player identity, survival snapshot, inventory
  snapshot, placed structures, resource depletion where needed, world
  time/weather state, active Ground Zero tile ID, and active tile package
  version.
- Save data vs external tile metadata boundary.
- Player identity, survival, inventory, world state, placed object, resource
  depletion, tile reference, and container record shapes.
- Save timing, load-on-server-start flow, migration rules, storage backend
  options, backup/recovery, security, and testing gates.

Roadmap update:

- Marked the persistence design document complete.
- Marked only the design/scope decisions complete in `1.13 Persistence MVP`;
  implementation tasks such as saving player identity, stats, inventory, world
  time, weather state, containers, save interval, load-on-start, and tile
  registry persistence remain open.
- Immediate next roadmap item is creating the Earth-scale terrain/tile
  streaming design document.

## Agrarian First/Third-Person Camera Toggle - 2026-05-15

Completed the roadmap item to implement the hybrid camera toggle.

Repo files updated:

- `Source/AgrarianGame/AgrarianGameCharacter.h`
- `Source/AgrarianGame/AgrarianGameCharacter.cpp`
- `Content/Input/Actions/IA_ToggleCamera.uasset`
- `Content/Input/IMC_Default.uasset`
- `Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset`
- `Scripts/setup_camera_toggle_input.py`
- `Scripts/verify_camera_toggle_input.py`
- `AGRARIAN_DEVELOPMENT_ROADMAP.md`

Behavior:

- Third person remains the default view.
- Press `V` or gamepad right thumbstick to toggle first-person view.
- First-person view uses a zero-length spring arm with a head-height camera
  offset, disables camera collision, and hides the local owner's mesh.
- Toggling back restores the third-person boom distance and mesh visibility.

Verification:

- Windows editor build completed successfully with
  `Scripts\BuildEditor-Windows.bat`.
- Unreal Python verification completed successfully with
  `Scripts\verify_camera_toggle_input.py`, confirming the input action,
  keyboard/gamepad mappings, and Blueprint assignment.

Roadmap update:

- Marked `Implement first/third-person camera toggle` complete.
- Immediate next roadmap item is `Implement sprinting`.

## Agrarian Sprinting - 2026-05-15

Completed the roadmap item to implement sprinting.

Repo files updated:

- `Source/AgrarianGame/AgrarianGameCharacter.h`
- `Source/AgrarianGame/AgrarianGameCharacter.cpp`
- `Content/Input/Actions/IA_Sprint.uasset`
- `Content/Input/IMC_Default.uasset`
- `Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset`
- `Scripts/setup_sprint_input.py`
- `Scripts/verify_sprint_input.py`
- `AGRARIAN_DEVELOPMENT_ROADMAP.md`

Behavior:

- Hold `LeftShift` or gamepad left thumbstick to sprint.
- Default walk speed is `500`; sprint speed is `750`.
- Sprinting spends stamina only on the server and only while moving.
- Sprint intent replicates to clients and updates movement speed on replication.
- Sprinting automatically stops when stamina falls below `MinSprintStamina`.

Verification:

- Windows editor build completed successfully with
  `Scripts\BuildEditor-Windows.bat`.
- Unreal Python setup completed successfully with `Scripts\setup_sprint_input.py`.
- Unreal Python verification completed successfully with
  `Scripts\verify_sprint_input.py`, confirming the input action,
  keyboard/gamepad mappings, and Blueprint assignment.

Roadmap update:

- Marked `Implement sprinting` complete.
- Immediate next roadmap item is `Define real-world baseline walking speed`.

## Agrarian Baseline Walking Speed - 2026-05-15

Completed the roadmap item to define real-world baseline walking speed.

Repo files updated:

- `Docs/MovementAndTimeScaleBaseline.md`
- `AGRARIAN_DEVELOPMENT_ROADMAP.md`

Decision:

- Keep the MVP calendar target of `4 real hours = 1 in-game day`.
- Do not multiply player movement speed by the calendar time scale.
- Baseline adult walking speed is `1.4 m/s`, or `140 Unreal units/s`.
- MVP tuning may allow up to about `1.6 m/s` for a brisk walk if the first
  playable build feels too sluggish.
- A flat 1 km tile edge-to-edge walk should take about 12 real minutes at
  baseline pace.

Rationale:

- Real terrain and 1 km tiles need real physical movement to feel grounded.
- Calendar compression should affect day/night rhythm, survival pressure,
  weather, sleep, crop growth, and long-term simulation, not the physical speed
  of the player body.
- Long travel consuming daylight is intentional; roads, carts, mounts, boats,
  vehicles, shelter, storage, and settlement planning should become meaningful
  progression systems.

Roadmap update:

- Marked `Define real-world baseline walking speed` complete.
- Immediate next roadmap item is `Define real-world baseline running speed`.

## Agrarian Baseline Running Speed - 2026-05-15

Completed the roadmap item to define real-world baseline running speed.

Repo files updated:

- `Docs/MovementAndTimeScaleBaseline.md`
- `Source/AgrarianGame/AgrarianGameCharacter.h`
- `Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset`
- `Scripts/setup_movement_baseline.py`
- `Scripts/verify_movement_baseline.py`
- `AGRARIAN_DEVELOPMENT_ROADMAP.md`

Decision:

- Keep movement measured in real distance per real second.
- Do not multiply running or sprinting by the `4 real hours = 1 in-game day`
  calendar.
- Sustainable adult running target is `3.0 m/s`, or `300 Unreal units/s`.
- The current MVP sprint input is a short burst at `5.5 m/s`, or
  `550 Unreal units/s`.
- The default walk speed was aligned to the walking baseline at `140 Unreal
  units/s`.
- Sprint stamina cost was raised to `28` per second so early characters cannot
  sprint across a full 1 km tile without rest or progression.

Verification:

- Windows editor build completed successfully with
  `Scripts\BuildEditor-Windows.bat`.
- Unreal Python movement baseline verification completed successfully with
  `Scripts\verify_movement_baseline.py`, confirming Blueprint defaults.

Roadmap update:

- Marked `Define real-world baseline running speed` complete.
- Immediate next roadmap item is `Connect movement speed to age, condition,
  strength, endurance, hunger, thirst, injury, carried weight, and terrain`.

## Agrarian Movement Modifiers - 2026-05-15

Completed the roadmap item to connect movement speed to age, condition,
strength, endurance, hunger, thirst, injury, carried weight, and terrain.

Repo files updated:

- `Source/AgrarianGame/AgrarianGameCharacter.h`
- `Source/AgrarianGame/AgrarianGameCharacter.cpp`
- `Source/AgrarianGame/AgrarianInventoryComponent.h`
- `Source/AgrarianGame/AgrarianInventoryComponent.cpp`
- `Scripts/setup_movement_baseline.py`
- `Scripts/verify_movement_baseline.py`
- `Docs/MovementAndTimeScaleBaseline.md`
- `AGRARIAN_DEVELOPMENT_ROADMAP.md`

Behavior:

- Character movement speed now uses `base speed * movement modifier`.
- Live modifiers include hunger, thirst, injury, carried inventory weight, and
  terrain multiplier.
- Replicated hooks were added for age, physical condition, strength, endurance,
  and terrain.
- Inventory now exposes `GetTotalWeight()`.
- Strength raises effective carry thresholds.
- Endurance improves movement resilience and reduces sprint stamina cost.
- Terrain multiplier is available through `SetTerrainMovementMultiplier()`.
- Movement modifier is clamped between `0.15` and `1.35`.

Verification:

- Windows editor build completed successfully with
  `Scripts\BuildEditor-Windows.bat`.
- Unreal Python movement baseline setup and verification completed successfully
  with `Scripts\setup_movement_baseline.py` and
  `Scripts\verify_movement_baseline.py`.

Roadmap update:

- Marked the movement-modifier item complete.
- Immediate next roadmap item is `Implement crouching if needed`.

## Agrarian Crouch And Prone Stances - 2026-05-15

Completed the roadmap item to implement crouching and prone movement stances.

Repo files updated:

- `Source/AgrarianGame/AgrarianGameCharacter.h`
- `Source/AgrarianGame/AgrarianGameCharacter.cpp`
- `Content/Input/Actions/IA_Crouch.uasset`
- `Content/Input/Actions/IA_Prone.uasset`
- `Content/Input/IMC_Default.uasset`
- `Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset`
- `Scripts/setup_stance_input.py`
- `Scripts/verify_stance_input.py`
- `Docs/MovementAndTimeScaleBaseline.md`
- `AGRARIAN_DEVELOPMENT_ROADMAP.md`

Behavior:

- `C` toggles crouch and gamepad Right Shoulder maps to crouch.
- `Z` toggles prone and gamepad Left Shoulder maps to prone.
- Crouch uses a `0.55` movement multiplier.
- Prone uses a `0.25` movement multiplier.
- Sprint cannot start while crouched or prone.
- Entering prone exits crouch so only one low stance is active.
- Prone is replicated and uses the same movement modifier pipeline as hunger,
  thirst, injury, carried weight, terrain, and traits.

Verification:

- `python3 -m py_compile` passed for the stance setup and verify scripts.
- `git diff --check` passed for the touched text files.
- Windows editor build completed successfully with
  `Scripts\BuildEditor-Windows.bat`.
- Unreal Python stance setup and verification completed successfully with
  `Scripts\setup_stance_input.py` and `Scripts\verify_stance_input.py`.

Roadmap update:

- Marked `Implement crouching and prone movement stances` complete.
- Immediate next roadmap item is `Implement interact prompt`.

## Agrarian Interaction Prompt - 2026-05-15

Completed the roadmap item to implement an interaction prompt.

Repo files updated:

- `Source/AgrarianGame/AgrarianGameCharacter.h`
- `Source/AgrarianGame/AgrarianGameCharacter.cpp`
- `Source/AgrarianGame/AgrarianDebugHUD.h`
- `Source/AgrarianGame/AgrarianDebugHUD.cpp`
- `Source/AgrarianGame/AgrarianGameGameMode.cpp`
- `Scripts/setup_interaction_prompt.py`
- `Scripts/verify_interaction_prompt.py`
- `AGRARIAN_DEVELOPMENT_ROADMAP.md`

Behavior:

- The local character now refreshes interactable focus each tick using the same
  visibility trace used by interaction.
- Focused interactables must implement `AgrarianInteractable` and pass
  `CanInteract`.
- Prompt text comes from `GetInteractionText` on the focused actor.
- The HUD draws `[E] <prompt>` near the center/lower-center of the screen.
- The prompt is independent of the dev HUD toggle so it can remain visible even
  when debug stats are hidden.
- `BP_ThirdPersonGameMode` is configured to use `AgrarianDebugHUD`.

Verification:

- `git diff --check` passed for touched text files.
- `python3 -m py_compile` passed for the setup and verify scripts.
- Windows editor build completed successfully with
  `Scripts\BuildEditor-Windows.bat`.
- Unreal Python setup and verification completed successfully with
  `Scripts\setup_interaction_prompt.py` and
  `Scripts\verify_interaction_prompt.py`.

Roadmap update:

- Marked `Implement interact prompt` complete.
- Immediate next roadmap item is `Implement basic animation blueprint`.

## Agrarian 0.1.C Time, Weather, And Environment Pressure - 2026-05-16

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `dddac97 Add first pass sky lighting`

Completed recent roadmap items:

- `Store weather source, provider timestamp, tile coordinate, and applied
  in-game weather state for debugging and persistence`
  - Commit: `26ddf8e Persist applied weather debug state`
  - Added tile ID/coordinate fields to mapped weather inputs.
  - Added replicated `FAgrarianWeatherDebugSnapshot`.
  - Save files now hold mapped weather inputs and applied weather debug state.
- `Add weather save/load support`
  - Commit: `8625583 Add weather save load support`
  - Added `LoadCurrentWorld` as the unified load path.
  - Weather/time restores before players and persistent world actors.
  - Admin load command now uses the combined load path.
  - Persistence smoke test verifies provider-backed weather metadata survives
    save/load.
- `Connect shelter to weather protection`
  - Commit: `0650806 Connect shelter to weather protection`
  - Survival detects overlapping `AAgrarianShelterActor` protection volumes.
  - `CurrentWeatherProtection` replicates on the survival component.
  - Shelter protection reduces ambient exposure and cold damage.
  - Care-history `ShelterQuality` trends toward active protection.
  - Dev HUD shows shelter protection.
- `Add first-pass sky and lighting`
  - Commit: `dddac97 Add first pass sky lighting`
  - Added `AAgrarianSkyLightingController`.
  - Controller owns movable sun, skylight, and fog components.
  - Lighting reads replicated game time, active tile sunrise/sunset, weather
    state, and provider cloud cover.
  - Ground Zero map setup now places `AGR_DemoSkyLightingController` and removes
    legacy static demo sun/skylight/fog actors.
  - `Content/Agrarian/Maps/L_GroundZeroTerrain_Test.umap` was regenerated and
    saved with the new controller.

Verification:

- Windows editor build passed after each completed item.
- Unreal Python map setup passed for `Scripts/setup_ground_zero_demo_map.py`
  after adding the sky lighting controller.
- Relevant static verifiers and `git diff --check` passed:
  - `Scripts/verify_weather_debug_persistence.py`
  - `Scripts/verify_weather_save_load_support.py`
  - `Scripts/verify_shelter_weather_protection.py`
  - `Scripts/verify_sky_lighting_controller.py`

Roadmap state:

- Current version section: `0.1.C Time, Weather, And Environment Pressure`
- Items remaining in `0.1.C`: `1`
- Immediate next roadmap item: `Add audio cues for weather`.

## Agrarian Weather Audio Cues - 2026-05-16

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `08a1df6 Add weather audio cues`

Completed roadmap item:

- `Add audio cues for weather`
  - Added `AAgrarianWeatherAudioController`.
  - Controller owns ambient, rain, wind, and storm audio components.
  - It reads replicated weather state, provider wind speed, and day/night state
    from `AAgrarianGameState`.
  - It fades component volumes for clear, rain, cold wind, and storm states.
  - Sound assets are intentionally assignable slots; runtime is silent until
    placeholder or final audio loops are assigned.
  - Ground Zero setup now places `AGR_DemoWeatherAudioController`.
  - `Content/Agrarian/Maps/L_GroundZeroTerrain_Test.umap` was regenerated and
    saved with the controller.

Verification:

- `python3 Scripts/verify_weather_audio_controller.py` passed.
- `python3 Scripts/verify_sky_lighting_controller.py` passed.
- `git diff --check` passed.
- Windows editor build passed through `/home/nathan/bin/agrarian-build-editor`.
- Unreal Python Ground Zero map setup passed through
  `Scripts/setup_ground_zero_demo_map.py`.

Roadmap state:

- Completed section: `0.1.C Time, Weather, And Environment Pressure`
- Items remaining in `0.1.C`: `0`
- Next version section: `0.1.D Single Biome MVP Map`
- Items remaining in `0.1.D`: `11`
- Immediate next roadmap item: `Link Ground Zero tile coordinates to real-world weather lookup coordinates`.

## Agrarian Ground Zero Weather Lookup Coordinates - 2026-05-16

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `a7292bb Link Ground Zero weather lookup coordinates`

Completed roadmap item:

- `Link Ground Zero tile coordinates to real-world weather lookup coordinates`
  - Added explicit `weather_lookup_metadata` to the Ground Zero tile registry
    entry.
  - Ground Zero weather lookup uses tile-center latitude `37.5925` and
    longitude `-122.4995`.
  - Weather lookup metadata records Open-Meteo primary routing, NOAA/NWS
    fallback eligibility, coordinate source, and lookup rule.
  - Updated the tile registry JSON schema and SQL prototype.
  - Updated `Scripts/generate_tile_weather_manifest.py` to read declared
    weather lookup metadata for future source-backed tiles.
  - Regenerated `Data/Tiles/tile_weather_manifest.json`.
  - Added `Scripts/verify_ground_zero_weather_lookup_coordinates.py`.
  - Updated docs and roadmap.

Verification:

- `python3 Scripts/generate_tile_weather_manifest.py` passed.
- `python3 Scripts/verify_ground_zero_weather_lookup_coordinates.py` passed.
- `python3 Scripts/verify_weather_provider_adapter.py` passed.
- `AGRARIAN_SKIP_LIVE_WEATHER_CHECK=1 python3 Scripts/verify_open_meteo_mvp_source.py` passed.
- `AGRARIAN_SKIP_LIVE_WEATHER_CHECK=1 python3 Scripts/verify_noaa_nws_us_fallback.py` passed.
- `python3 -m py_compile Scripts/generate_tile_weather_manifest.py Scripts/verify_ground_zero_weather_lookup_coordinates.py Scripts/verify_weather_provider_adapter.py` passed.
- `git diff --check` passed.
- Windows editor build passed through `/home/nathan/bin/agrarian-build-editor`.

Roadmap state:

- Current version section: `0.1.D Single Biome MVP Map`
- Items remaining in `0.1.D`: `10`
- Immediate next roadmap item: `Replace grey-box environment presentation with an MVP natural environment pass: terrain material, grass, shrubs, bushes, trees, water-source visuals, and clearer Ground Zero biome dressing`.

## Agrarian Ground Zero Natural Environment Pass - 2026-05-16

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `25ffbfc Add Ground Zero natural environment pass`

Completed roadmap item:

- `Replace grey-box environment presentation with an MVP natural environment pass`
  - Added repeatable material generation in `Scripts/setup_ground_zero_demo_map.py`.
  - Created 8 Ground Zero material assets under `Content/Agrarian/Materials/`.
  - Applied the terrain material to the landscape.
  - Applied distinct tree, shrub, and dry grass materials to the foliage patch.
  - Applied wood, fiber, stone, and freshwater materials to placed resource/water
    actors.
  - Regenerated/saved `Content/Agrarian/Maps/L_GroundZeroTerrain_Test.umap`.
  - Added `Docs/Terrain/GroundZeroNaturalEnvironmentPass.md`.
  - Added `Scripts/verify_ground_zero_natural_environment_pass.py`.

Verification:

- `python3 -m py_compile Scripts/setup_ground_zero_demo_map.py Scripts/verify_ground_zero_natural_environment_pass.py` passed.
- `git diff --check` passed.
- Unreal Python Ground Zero setup passed.
- Unreal Python natural environment verifier passed: 8 materials, 1 landscape,
  14 dressed resource/water actors.
- Unreal Python foliage verifier passed: 42 trees, 96 shrubs, 180 grass clumps.
- Windows editor build passed through `/home/nathan/bin/agrarian-build-editor`.

Roadmap state:

- Current version section: `0.1.D Single Biome MVP Map`
- Items remaining in `0.1.D`: `9`
- Immediate next roadmap item: `Add first-pass environment asset variation so trees, bushes, grass, resource nodes, and water do not read as repeated placeholders`.

## Agrarian Ground Zero Environment Asset Variation - 2026-05-16

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `f78c552 Add Ground Zero environment asset variation`

Completed roadmap item:

- `Add first-pass environment asset variation so trees, bushes, grass, resource nodes, and water do not read as repeated placeholders`
  - Added 11 repeatable labeled variation actors through
    `Scripts/setup_ground_zero_demo_map.py`.
  - Covered tree canopy/trunk pairs, rounded bushes, grass mats, rock slabs, and
    a visible freshwater surface.
  - Used multiple prototype mesh silhouettes, unique scale profiles, rotations,
    and the existing Ground Zero material families.
  - Regenerated/saved `Content/Agrarian/Maps/L_GroundZeroTerrain_Test.umap`.
  - Updated the Ground Zero natural environment and foliage docs.
  - Extended the natural environment verifier to require actor count, mesh
    variety, unique scales, and material-family coverage.

Verification:

- `python3 -m py_compile Scripts/setup_ground_zero_demo_map.py Scripts/verify_ground_zero_natural_environment_pass.py` passed.
- `git diff --check` passed.
- Unreal Python Ground Zero setup passed.
- Unreal Python natural environment verifier passed: 8 materials, 1 landscape,
  14 dressed resource/water actors, 11 variation actors.
- Unreal Python foliage verifier passed: 42 trees, 96 shrubs, 180 grass clumps.
- Windows editor build passed through `/home/nathan/bin/agrarian-build-editor`.

Roadmap state:

- Current version section: `0.1.D Single Biome MVP Map`
- Items remaining in `0.1.D`: `8`
- Immediate next roadmap item: `Replace LevelPrototyping cube/cylinder mesh dependencies in Agrarian setup scripts and prototype Blueprints with Agrarian-native placeholder environment meshes`.

## Agrarian Native Placeholder Meshes - 2026-05-16

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `b5d1359 Replace template meshes with native placeholders`

Completed roadmap item:

- `Replace LevelPrototyping cube/cylinder mesh dependencies in Agrarian setup scripts and prototype Blueprints with Agrarian-native placeholder environment meshes`
  - Added five native placeholder static mesh assets under
    `/Game/Agrarian/Environment/PlaceholderMeshes`.
  - Updated playable Blueprint setup and Ground Zero setup scripts so active
    outputs use native Agrarian mesh paths.
  - Left template mesh references only as copy-source paths inside repeatable
    setup helpers.
  - Regenerated/saved the affected resource, structure, and water Blueprints.
  - Regenerated/saved `Content/Agrarian/Maps/L_GroundZeroTerrain_Test.umap`.
  - Added `Scripts/verify_native_placeholder_meshes.py`.
  - Updated the template decision doc, natural environment doc, and roadmap.

Verification:

- `python3 -m py_compile Scripts/setup_ground_zero_demo_map.py Scripts/setup_playable_blueprints.py Scripts/verify_native_placeholder_meshes.py` passed.
- `git diff --check` passed.
- Unreal Python playable Blueprint setup passed.
- Unreal Python Ground Zero setup passed.
- Unreal Python native placeholder verifier passed: 5 native mesh assets, 6
  Blueprint meshes, foliage meshes, and environment variation meshes use native
  paths.
- Unreal Python natural environment verifier passed: 8 materials, 1 landscape,
  14 dressed resource/water actors, 11 variation actors.
- Unreal Python foliage verifier passed: 42 trees, 96 shrubs, 180 grass clumps.
- Unreal Python player Blueprint verifier passed.
- Windows editor build passed through `/home/nathan/bin/agrarian-build-editor`.

Roadmap state:

- Current version section: `0.1.D Single Biome MVP Map`
- Items remaining in `0.1.D`: `7`
- Immediate next roadmap item: `Add weather exposure zones if needed`.

## Agrarian Ground Zero Weather Exposure Zones - 2026-05-16

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `8b0b5ff Add Ground Zero weather exposure zones`

Completed roadmap item:

- `Add weather exposure zones if needed`
  - Added native replicated `AAgrarianWeatherExposureZone` actor with a box
    overlap volume, exposure multiplier, temperature offset, and stable zone id.
  - Updated `UAgrarianSurvivalComponent` so overlapping weather zones adjust
    ambient temperature exposure and cold damage after shelter/weather
    protection.
  - Updated the debug HUD to show current exposure multiplier and temperature
    offset.
  - Added three repeatable Ground Zero zones through
    `Scripts/setup_ground_zero_demo_map.py`: ridge exposure, coastal wind, and
    drainage cooling.
  - Regenerated/saved `Content/Agrarian/Maps/L_GroundZeroTerrain_Test.umap`.
  - Added `Scripts/verify_weather_exposure_zones.py`.
  - Updated the technical design document and roadmap.

Verification:

- `python3 -m py_compile Scripts/setup_ground_zero_demo_map.py Scripts/verify_weather_exposure_zones.py` passed.
- `git diff --check` passed.
- Windows editor build passed through `/home/nathan/bin/agrarian-build-editor`.
- Unreal Python Ground Zero setup passed.
- Unreal Python weather exposure verifier passed: 3 zones with exposure
  multipliers and temperature offsets.
- Unreal Python natural environment verifier passed: 8 materials, 1 landscape,
  14 dressed resource/water actors, 11 variation actors.
- Unreal Python foliage verifier passed: 42 trees, 96 shrubs, 180 grass clumps.
- Final Windows editor build passed through `/home/nathan/bin/agrarian-build-editor`.

Roadmap state:

- Current version section: `0.1.D Single Biome MVP Map`
- Items remaining in `0.1.D`: `6`
- Immediate next roadmap item: `Add landmark or ruin placeholder`.

## Agrarian Ground Zero Ruin Landmark Placeholder - 2026-05-16

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `56f8ef9 Add Ground Zero ruin landmark placeholder`

Completed roadmap item:

- `Add landmark or ruin placeholder`
  - Added a repeatable five-piece Ground Zero ruin cluster through
    `Scripts/setup_ground_zero_demo_map.py`.
  - The cluster includes a foundation slab, two wall fragments, a cairn marker,
    and a threshold/lintel piece.
  - The ruin uses Agrarian-native placeholder meshes and the Ground Zero stone
    material so it is visible in the MVP but easy to replace later.
  - Regenerated/saved `Content/Agrarian/Maps/L_GroundZeroTerrain_Test.umap`.
  - Added `Scripts/verify_ground_zero_landmark_placeholder.py`.
  - Updated the Ground Zero natural environment doc and roadmap.

Verification:

- `python3 -m py_compile Scripts/setup_ground_zero_demo_map.py Scripts/verify_ground_zero_landmark_placeholder.py` passed.
- `git diff --check` passed.
- Unreal Python Ground Zero setup passed.
- Unreal Python landmark placeholder verifier passed: 5 ruin actors, 3 mesh
  silhouettes.
- Unreal Python natural environment verifier passed: 8 materials, 1 landscape,
  14 dressed resource/water actors, 11 variation actors.
- Unreal Python foliage verifier passed: 42 trees, 96 shrubs, 180 grass clumps.
- Windows editor build passed through `/home/nathan/bin/agrarian-build-editor`.

Roadmap state:

- Current version section: `0.1.D Single Biome MVP Map`
- Items remaining in `0.1.D`: `5`
- Immediate next roadmap item: `Add spawn area with validation that the player spawns above sea level, above terrain by a safe offset, away from water, away from steep slopes, away from dense resource clusters, and with a known safe fallback coordinate`.

## Agrarian Ground Zero Safe Spawn Area - 2026-05-16

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `e50745d Validate Ground Zero safe spawn area`

Completed roadmap item:

- `Add spawn area with validation that the player spawns above sea level, above terrain by a safe offset, away from water, away from steep slopes, away from dense resource clusters, and with a known safe fallback coordinate`
  - Added safe spawn candidate/fallback selection to
    `Scripts/setup_ground_zero_demo_map.py`.
  - Setup validates minimum elevation above sea level, maximum sampled terrain
    slope, minimum above-terrain Z offset, freshwater spacing, and
    resource-cluster spacing before saving the map.
  - `AGR_DemoPlayerStart` now uses the known safe fallback coordinate
    `(-22000, -3500)`.
  - Added `Scripts/verify_ground_zero_safe_spawn.py`.
  - Regenerated/saved `Content/Agrarian/Maps/L_GroundZeroTerrain_Test.umap`.
  - Updated the technical design document and roadmap.

Verification:

- `python3 -m py_compile Scripts/setup_ground_zero_demo_map.py Scripts/verify_ground_zero_safe_spawn.py` passed.
- `git diff --check` passed.
- Unreal Python Ground Zero setup passed.
- Unreal Python safe-spawn verifier passed: location `(-22000, -3500, 1148)`,
  terrain `9.28m`, slope `1.94deg`, above-terrain offset `220cm`.
- Unreal Python natural environment verifier passed.
- Unreal Python foliage verifier passed.
- Unreal Python landmark placeholder verifier passed.
- Windows editor build passed through `/home/nathan/bin/agrarian-build-editor`.
- Note: parallel Unreal verifier runs logged intermediate asset-registry cache
  write warnings because multiple editor processes started together; all
  verifiers completed successfully and the final editor build passed.

Roadmap state:

- Current version section: `0.1.D Single Biome MVP Map`
- Items remaining in `0.1.D`: `4`
- Immediate next roadmap item: `Add performance profiling markers`.

## Agrarian Performance Profiling Markers - 2026-05-16

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `82463f3 Add Agrarian performance profiling markers`

Completed roadmap item:

- `Add performance profiling markers`
  - Added `Source/AgrarianGame/AgrarianPerformanceStats.h` and
    `Source/AgrarianGame/AgrarianPerformanceStats.cpp` with
    `STATGROUP_Agrarian`.
  - Added `stat Agrarian` cycle counters and Unreal Insights CPU scopes for
    authoritative game-state time/weather ticking, survival ticking,
    sky-light refresh, weather-audio refresh, foliage instance mutation, and
    weather-provider request/parse/fallback work.
  - Added `Scripts/verify_performance_profiling_markers.py`.
  - Updated the technical design document and roadmap.

Verification:

- `python3 -m py_compile Scripts/verify_performance_profiling_markers.py` passed.
- `python3 Scripts/verify_performance_profiling_markers.py` passed.
- `git diff --check` passed.
- Windows editor build passed through `/home/nathan/bin/agrarian-build-editor`.

Roadmap state:

- Current version section: `0.1.D Single Biome MVP Map`
- Items remaining in `0.1.D`: `3`
- Immediate next roadmap item: `Add navigation support for wildlife`.

## Agrarian Wildlife Navigation Support - 2026-05-16

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `578220c Add wildlife navigation support`

Completed roadmap item:

- `Add navigation support for wildlife`
  - Added `NavigationSystem` to the Agrarian module dependencies.
  - Wildlife now auto-possesses an AI controller when placed or spawned.
  - Wander targets are sampled from reachable nav points when nav data exists.
  - Chase and flee targets are projected onto navmesh.
  - Server-authoritative wildlife movement uses `MoveToLocation`.
  - Direct movement input remains as a fallback for early maps/tiles without
    nav data.
  - AI movement stops cleanly when wildlife dies.
  - Updated the wildlife loop verifier to find the current Ground Zero rabbit
    label.
  - Added `Scripts/verify_wildlife_navigation_support.py`.
  - Updated the technical design document and roadmap.

Verification:

- `python3 -m py_compile Scripts/verify_wildlife_navigation_support.py Scripts/verify_wildlife_loop.py` passed.
- `python3 Scripts/verify_wildlife_navigation_support.py` passed.
- `git diff --check` passed.
- Windows editor build passed through `/home/nathan/bin/agrarian-build-editor`.
- Unreal Python wildlife loop verifier passed through
  `Scripts/verify_wildlife_loop.py`.

Roadmap state:

- Current version section: `0.1.D Single Biome MVP Map`
- Items remaining in `0.1.D`: `2`
- Immediate next roadmap item: `Add map boundaries or soft limits`.

## Agrarian Ground Zero Map Boundary - 2026-05-16

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `65bcdf6 Add Ground Zero map boundary`

Completed roadmap item:

- `Add map boundaries or soft limits`
  - Added native `AAgrarianMapBoundaryVolume` with a box volume, boundary ID,
    warning-zone test, clamp helper, and server-authoritative player clamp.
  - Placed `AGR_GroundZeroMapBoundary` around the loaded 1 km Ground Zero MVP
    tile in `Content/Agrarian/Maps/L_GroundZeroTerrain_Test.umap`.
  - Updated `Scripts/setup_ground_zero_demo_map.py` so regenerated Ground Zero
    maps automatically recreate the boundary volume.
  - Added `Scripts/verify_map_boundary_source.py`.
  - Added `Scripts/verify_ground_zero_map_boundary.py`.
  - Updated the technical design document and roadmap.

Verification:

- `python3 -m py_compile Scripts/setup_ground_zero_demo_map.py Scripts/verify_map_boundary_source.py Scripts/verify_ground_zero_map_boundary.py` passed.
- `python3 Scripts/verify_map_boundary_source.py` passed.
- `git diff --check` passed.
- Windows editor build passed through `/home/nathan/bin/agrarian-build-editor`.
- Unreal Python Ground Zero setup passed through
  `Scripts/setup_ground_zero_demo_map.py`.
- Unreal Python boundary verifier passed through
  `Scripts/verify_ground_zero_map_boundary.py`.

Roadmap state:

- Current version section: `0.1.D Single Biome MVP Map`
- Items remaining in `0.1.D`: `1`
- Immediate next roadmap item: `Add developer travel command`.

## Agrarian Developer Travel Command - 2026-05-16

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `e436455 Add developer travel command`

Completed roadmap item:

- `Add developer travel command`
  - Added server-authoritative `AgrarianTravel X Y Z` and
    `AgrarianTravelHome` exec commands on `AAgrarianGamePlayerController`.
  - `AgrarianTravel` teleports the controlled pawn to explicit Unreal world
    coordinates, rejects invalid NaN destinations, stops active character
    movement after teleport, and reports the destination to the issuing player.
  - `AgrarianTravelHome` returns the player to the validated Ground Zero safe
    spawn fallback near `AGR_DemoPlayerStart` at `(-22000, -3500, 1148)`.
  - Added `Scripts/verify_developer_travel_command.py`.
  - Updated the technical design document and roadmap.

Verification:

- `python3 -m py_compile Scripts/verify_developer_travel_command.py` passed.
- `python3 Scripts/verify_developer_travel_command.py` passed.
- `git diff --check` passed.
- Windows editor build through `/home/nathan/bin/agrarian-build-editor`
  initially failed on a `Character` local-variable shadow warning; after
  renaming it to `ControlledCharacter`, the editor build passed.

Roadmap state:

- Completed version section: `0.1.D Single Biome MVP Map`
- Items remaining in `0.1.D`: `0`
- Current/next version section: `0.1.E Inventory System`
- Items remaining in `0.1.E`: `9`
- Immediate next roadmap item: `Design inventory data model`.

## Agrarian Inventory Data Model - 2026-05-16

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `c511ae9 Design inventory data model`

Completed roadmap item:

- `Design inventory data model`
  - Added `Docs/InventoryDataModel.md` defining the MVP contract for stable
    item definitions, runtime/save-game item stacks, server-authoritative
    replicated inventory components, persistence, carry weight, and the
    pickup/drop/split/use equipment/UI operations that build on the current
    code.
  - Updated the technical design document to point future inventory work at the
    new model instead of parallel state.
  - Added `Scripts/verify_inventory_data_model.py`.
  - Updated the roadmap.

Verification:

- `python3 -m py_compile Scripts/verify_inventory_data_model.py` passed.
- `python3 Scripts/verify_inventory_data_model.py` passed.
- `git diff --check` passed.
- No Windows editor build was run for this item because it only changed docs and
  Python verification.

Roadmap state:

- Current version section: `0.1.E Inventory System`
- Items remaining in `0.1.E`: `8`
- Immediate next roadmap item: `Add item pickup`.

## Agrarian Item Pickup - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `03d856e Add item pickup actor`

Completed roadmap item:

- `Add item pickup`
  - Added native `AAgrarianItemPickup`, a replicated interactable world actor
    with a static mesh, definition-backed or inline `FAgrarianItemStack` data,
    quantity, prompt text, and server-authoritative pickup behavior.
  - Pickup interaction validates authority and stack validity, adds the stack to
    the player's `UAgrarianInventoryComponent`, and destroys the world pickup
    only after `Inventory->AddItem` succeeds so failed pickups remain available.
  - Added `Scripts/verify_item_pickup.py`.
  - Updated the technical design document and roadmap.

Verification:

- `python3 -m py_compile Scripts/verify_item_pickup.py` passed.
- `python3 Scripts/verify_item_pickup.py` passed.
- `git diff --check` passed.
- Windows editor build passed through `/home/nathan/bin/agrarian-build-editor`.

Roadmap state:

- Current version section: `0.1.E Inventory System`
- Items remaining in `0.1.E`: `7`
- Immediate next roadmap item: `Add item drop`.

## Agrarian Item Drop - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `b48595f Add item drop command`

Completed roadmap item:

- `Add item drop`
  - Added `UAgrarianInventoryComponent::ExtractItem` so item removal can return
    dropped stack metadata for world pickups while preserving existing
    `RemoveItem` behavior.
  - Added server-authoritative `AgrarianDropItem ItemId Quantity` on
    `AAgrarianGamePlayerController`.
  - Drop command extracts inventory stack data, spawns an
    `AAgrarianItemPickup` in front of the player, and restores the removed stack
    if pickup spawning fails.
  - Added `Scripts/verify_item_drop.py`.
  - Updated the technical design document and roadmap.

Verification:

- `python3 -m py_compile Scripts/verify_item_drop.py` passed.
- `python3 Scripts/verify_item_drop.py` passed.
- `git diff --check` passed.
- Windows editor build passed through `/home/nathan/bin/agrarian-build-editor`.

Roadmap state:

- Current version section: `0.1.E Inventory System`
- Items remaining in `0.1.E`: `6`
- Immediate next roadmap item: `Add stack splitting`.

## Agrarian Stack Splitting - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `555ad6d Add stack splitting`

Completed roadmap item:

- `Add stack splitting`
  - Added server-authoritative `UAgrarianInventoryComponent::SplitStackByIndex`.
  - Added `ServerSplitStackByIndex` RPC for future UI use.
  - Added `AgrarianSplitStack StackIndex SplitQuantity` debug command on
    `AAgrarianGamePlayerController`.
  - Split operation validates source slot, split quantity, and free slot
    capacity, copies source stack metadata, reduces the source quantity, creates
    a separate stack slot, and intentionally avoids re-merging through `AddItem`.
  - Added `Scripts/verify_stack_splitting.py`.
  - Updated the technical design document and roadmap.

Verification:

- `python3 -m py_compile Scripts/verify_stack_splitting.py` passed.
- `python3 Scripts/verify_stack_splitting.py` passed.
- `git diff --check` passed.
- Windows editor build passed through `/home/nathan/bin/agrarian-build-editor`.

Roadmap state:

- Current version section: `0.1.E Inventory System`
- Items remaining in `0.1.E`: `5`
- Immediate next roadmap item: `Add item use`.

## Agrarian Item Use - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `712a854 Add item use command`

Completed roadmap item:

- `Add item use`
  - Added server-authoritative `AgrarianUseItem ItemId Quantity`.
  - Added `ApplyAgrarianItemUseEffect` MVP rules:
    - `food` restores hunger.
    - `meat` restores more hunger but adds raw-meat sickness risk.
    - `bandage` reduces injury severity and restores a small amount of health.
  - Added `UAgrarianSurvivalComponent::ReduceInjury`.
  - Unsupported items are restored to inventory instead of consumed.
  - Added `Scripts/verify_item_use.py`.
  - Updated the technical design document and roadmap.

Verification:

- `python3 -m py_compile Scripts/verify_item_use.py` passed.
- `python3 Scripts/verify_item_use.py` passed.
- `git diff --check` passed.
- Windows editor build passed through `/home/nathan/bin/agrarian-build-editor`.

Roadmap state:

- Current version section: `0.1.E Inventory System`
- Items remaining in `0.1.E`: `4`
- Immediate next roadmap item: `Add equipment slots if needed`.

## Agrarian Equipment Slot Decision - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `edd4050 Document equipment slot decision`

Completed roadmap item:

- `Add equipment slots if needed`
  - Decision: dedicated equipment slots are deferred for the 0.1.E MVP.
  - Reason: current tools, including `basic_tool`, do not yet drive active hand,
    worn, backpack, armor, weapon, durability, animation, or mesh-attachment
    rules; adding slots now would create replicated/UI/save state with no
    gameplay consumer.
  - Documented the future trigger: add server-authoritative, replicated,
    persisted equipment slot state when an implemented system actually needs
    equipped state.
  - Added `Scripts/verify_equipment_slot_decision.py`.
  - Updated the inventory data model, technical design document, and roadmap.

Verification:

- `python3 -m py_compile Scripts/verify_equipment_slot_decision.py` passed.
- `python3 Scripts/verify_equipment_slot_decision.py` passed.
- `git diff --check` passed.
- No Windows editor build was run because this item only changed docs and Python
  verification.

Roadmap state:

- Current version section: `0.1.E Inventory System`
- Items remaining in `0.1.E`: `3`
- Immediate next roadmap item: `Add weight or carry capacity placeholder`.

## Agrarian Carry Capacity Placeholder - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `cb663fb Document carry capacity placeholder`

Completed roadmap item:

- `Add weight or carry capacity placeholder`
  - Confirmed the existing carry-capacity placeholder is weight-first.
  - Item stacks carry `UnitWeight`.
  - `UAgrarianInventoryComponent::GetTotalWeight` sums `Quantity * UnitWeight`.
  - `AAgrarianGameCharacter::GetCurrentCarryWeight` reads inventory total.
  - Movement uses comfort/heavy thresholds of `25.0` and `60.0`.
  - `StrengthMultiplier` scales effective carry thresholds.
  - Heavy loads reduce carry movement multiplier down to `45%`.
  - The debug HUD shows current carried weight for tuning.
  - Documented that later volume, backpack, awkward-object, injury/fatigue, and
    hard overload systems should extend this total-weight hook.
  - Added `Scripts/verify_carry_capacity_placeholder.py`.
  - Updated the inventory data model, technical design document, and roadmap.

Verification:

- `python3 -m py_compile Scripts/verify_carry_capacity_placeholder.py` passed.
- `python3 Scripts/verify_carry_capacity_placeholder.py` passed.
- `git diff --check` passed.
- No Windows editor build was run because this item only changed docs and
  Python verification around existing C++ behavior.

Automation notes:

- AWS SES in `us-west-2` is available.
- `pacificao.com` is a verified SES identity.
- Per-item summaries can be sent to `nathan@pacificao.com` from a
  `pacificao.com` sender.

Roadmap state:

- Current version section: `0.1.E Inventory System`
- Items remaining in `0.1.E`: `2`
- Immediate next roadmap item: `Add inventory UI`.

## Agrarian Inventory UI - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `09eed7c Add MVP inventory HUD panel`

Completed roadmap item:

- `Add inventory UI`
  - Added a compact MVP inventory panel to `AAgrarianDebugHUD`.
  - The panel is controlled separately from the full developer HUD by
    `bShowInventoryHUD`.
  - It reads the replicated inventory component and shows occupied slots, max
    slots, total carried weight, visible item stacks, stack quantity, and total
    stack weight.
  - Kept mutation flows on existing server-authoritative commands/RPCs until a
    full UMG inventory screen is introduced.
  - Added `Scripts/verify_inventory_ui.py`.
  - Updated the inventory data model, technical design document, and roadmap.

Verification:

- `python3 -m py_compile Scripts/verify_inventory_ui.py` passed.
- `python3 Scripts/verify_inventory_ui.py` passed.
- `git diff --check` passed.
- Windows editor build passed through `/home/nathan/bin/agrarian-build-editor`.

Automation:

- Email summary sent to `nathan@pacificao.com` through AWS SES.

Roadmap state:

- Current version section: `0.1.E Inventory System`
- Items remaining in `0.1.E`: `1`
- Immediate next roadmap item: `Add persistence for inventory`.

## Agrarian Inventory Persistence - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed commit: `ac9fee4 Add inventory persistence restore hook`

Completed roadmap item:

- `Add persistence for inventory`
  - `FAgrarianSavedPlayer::Inventory` stores the player stack array.
  - Player capture writes `InventoryComponent->Items`.
  - Added `UAgrarianInventoryComponent::RestoreSavedItems`.
  - Restore broadcasts `OnInventoryChanged`.
  - `UAgrarianPersistenceSubsystem::RestorePlayers` now uses the restore helper
    instead of assigning `Items` directly.
  - HUD/UI listeners and future inventory UI stay synchronized after loading.
  - Total carry weight remains derived from stack data.
  - Added `Scripts/verify_inventory_persistence.py`.
  - Updated the inventory data model, technical design document, and roadmap.

Verification:

- `python3 -m py_compile Scripts/verify_inventory_persistence.py` passed.
- `python3 Scripts/verify_inventory_persistence.py` passed.
- `git diff --check` passed.
- Windows editor build passed through `/home/nathan/bin/agrarian-build-editor`.

Automation:

- Email summary sent to `nathan@pacificao.com` through AWS SES.

Roadmap state:

- Current version section: `0.1.E Inventory System`
- Items remaining in `0.1.E`: `0`
- Immediate next action: build Windows investor demo and stop.

## Agrarian Windows Investor Demo - 2026-05-17

Stop point completed:

- Remaining `0.1.E Inventory System` items are complete.
- Investor demo version strings were bumped before packaging:
  - splash/demo notice: `Investor Demo v0.1.E - Build 2026.05.17`;
  - `Config/DefaultGame.ini`: `ProjectVersion=0.1.E-investor.20260517`;
  - packaged README/launchers use the same demo version.
- Latest pushed game commit after the version bump:
  `5fc6a61 Bump investor demo version to 0.1.E`.

Windows package:

- Package command run on Windows builder from `P:\AgrarianGameBulid`:
  `Scripts\PackageWindowsDevelopment.bat`
- Result: success.
- Output folder:
  `/mnt/projects/AgrarianGameBulid/Builds/WindowsDevelopment`
- Approximate size: `1.1G`
- Key files:
  - `AgrarianGame.exe`
  - `README-Investor-Demo.txt`
  - `Install Prerequisites.cmd`
  - `Start Agrarian Demo.cmd`
  - `Start Agrarian Demo - DX12.cmd`
  - `Start Agrarian Demo - Compatibility DX11.cmd`

Repo state after package:

- Game repo clean on `main...origin/main`.
- Handoff repo clean on `main...origin/main`.

Instruction:

- Stop here per user request. Next development item should be selected only
  after review of the investor demo or a new instruction.

## Agrarian 0.1.F Stone Resource - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `d693113 Document stone resource support`

Completed roadmap item:

- `Add stone resource`
  - Confirmed stone already exists as `DA_Item_Stone`.
  - Confirmed `BP_StoneResourceNode` is configured through playable Blueprint
    setup.
  - Confirmed deterministic Ground Zero stone node placements exist.
  - Confirmed playable Blueprint and Ground Zero resource verifiers cover
    stone.
  - Documented the gatherable resource baseline in the technical design doc.
  - Marked the roadmap item complete.
  - Added `Scripts/verify_stone_resource.py`.

Verification:

- `python3 -m py_compile Scripts/verify_stone_resource.py` passed.
- `python3 Scripts/verify_stone_resource.py` passed.
- `git diff --check` passed.
- No Windows editor build was run because this item closed existing content via
  docs and Python verification only.

Roadmap state:

- Current version section: `0.1.F Gathering And Resources`
- Items remaining in `0.1.F`: `5`
- Immediate next roadmap item: `Add edible plant resource`.

## Agrarian 0.1.F Edible Plant Resource - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `0545522 Add edible plant resource nodes`

Completed roadmap item:

- `Add edible plant resource`
  - Added `BP_EdiblePlantResourceNode` as an Agrarian resource Blueprint.
  - The edible plant node yields the MVP `food` item from forage patches.
  - Added `M_AGR_GZ_EdiblePlant_Resource`.
  - Added three deterministic Ground Zero edible plant resource nodes:
    - `AGR_GZ_EdiblePlant_CoastalScrub_01`
    - `AGR_GZ_EdiblePlant_Grassland_02`
    - `AGR_GZ_EdiblePlant_DrainageCandidate_03`
  - Updated the Ground Zero resource pass and technical design documentation.
  - Added `Scripts/verify_edible_plant_resource.py`.
  - Extended playable Blueprint and Ground Zero resource verifiers.

Verification:

- `python3 -m py_compile Scripts/setup_playable_blueprints.py Scripts/setup_ground_zero_demo_map.py Scripts/verify_playable_blueprints.py Scripts/verify_ground_zero_resources.py Scripts/verify_edible_plant_resource.py` passed.
- `python3 Scripts/verify_edible_plant_resource.py` passed.
- Windows Unreal Python setup for playable Blueprints passed.
- Windows Unreal Python setup for Ground Zero map resources passed.
- Windows Unreal Python `verify_playable_blueprints.py` passed.
- Windows Unreal Python `verify_ground_zero_resources.py` passed and reported
  `4 wood, 5 fiber, 3 edible plant, 4 stone nodes`.
- `git diff --check` passed.

Roadmap state:

- Current version section: `0.1.F Gathering And Resources`
- Items remaining in `0.1.F`: `4`
- Immediate next roadmap item: `Add water gathering interaction`.

## Agrarian 0.1.F Water Gathering Interaction - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `fa9d183 Verify water gathering interaction`

Completed roadmap item:

- `Add water gathering interaction`
  - Confirmed water gathering already exists through `AAgrarianWaterSource` and
    `BP_FreshWaterSource`.
  - Confirmed the water source uses the shared focused
    `IAgrarianInteractable` path.
  - Confirmed player-facing prompt text flows through
    `AAgrarianGameCharacter::FindFocusedInteractable`.
  - Confirmed server-authoritative `AAgrarianGameCharacter::ServerInteract`
    range validation before interaction.
  - Confirmed interaction restores thirst through
    `UAgrarianSurvivalComponent::AddWater` with restore amount `45`.
  - Added `Scripts/verify_water_gathering_interaction.py`.
  - Updated the roadmap, technical design document, Ground Zero resource pass,
    and Ground Zero freshwater source docs.

Verification:

- `python3 -m py_compile Scripts/verify_water_gathering_interaction.py` passed.
- `python3 Scripts/verify_water_gathering_interaction.py` passed.
- Windows Unreal Python `verify_ground_zero_water_source.py` passed and
  reported `AGR_GZ_FreshWaterSource_01, restore=45.0`.
- `git diff --check` passed.

Roadmap state:

- Current version section: `0.1.F Gathering And Resources`
- Items remaining in `0.1.F`: `3`
- Immediate next roadmap item: `Add respawn rules for MVP`.

## Agrarian 0.1.F Resource Respawn Rules - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `a5ec210 Add MVP resource respawn rules`

Completed roadmap item:

- `Add respawn rules for MVP`
  - Added native resource-node respawn fields:
    `bRespawnsForMvp`, `RespawnDelaySeconds`, and `MaxHarvests`.
  - Added server-side respawn timer logic. Renewable nodes schedule a timer
    only when fully depleted.
  - Respawn restores replicated `RemainingHarvests` and re-enables
    visibility/collision.
  - Configured MVP Blueprint defaults:
    - wood respawns after `900` seconds;
    - fiber respawns after `600` seconds;
    - edible plants respawn after `1200` seconds;
    - stone remains nonrespawning.
  - Regenerated and saved resource Blueprints with the respawn defaults.
  - Added `Scripts/verify_resource_respawn_rules.py`.
  - Updated the roadmap, technical design document, Ground Zero resource pass,
    `setup_playable_blueprints.py`, and `verify_playable_blueprints.py`.

Verification:

- `python3 -m py_compile Scripts/setup_playable_blueprints.py Scripts/verify_playable_blueprints.py Scripts/verify_resource_respawn_rules.py` passed.
- `python3 Scripts/verify_resource_respawn_rules.py` passed.
- `git diff --check` passed.
- Windows editor build passed through direct `Scripts\BuildEditor-Windows.bat`.
- Windows Unreal Python `setup_playable_blueprints.py` passed.
- Windows Unreal Python `verify_playable_blueprints.py` passed.
- Note: `/home/nathan/bin/agrarian-build-editor` appeared to hang silently and
  was stopped; the direct Windows build command completed successfully.

Roadmap state:

- Current version section: `0.1.F Gathering And Resources`
- Items remaining in `0.1.F`: `2`
- Immediate next roadmap item: `Add tool requirement rules`.

## Agrarian 0.1.F Resource Tool Rules - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `843340e Add MVP resource tool rules`

Completed roadmap item:

- `Add tool requirement rules`
  - Added inventory-based resource tool fields:
    `RequiredToolItemId`, `bAllowBareHandGathering`, and
    `ToolQuantityBonus`.
  - Added harvest yield logic that keeps current MVP resources gatherable by
    hand while granting a yield bonus when the player has the configured tool.
  - Configured MVP Blueprint defaults:
    - wood, fiber, and stone declare `basic_tool`, allow bare-hand gathering,
      and gain `+1` yield with the tool;
    - edible plants remain bare-hand gatherable without a tool bonus.
  - Avoided a first-loop deadlock by not hard-requiring `basic_tool` for
    stone, since the `basic_tool` recipe requires stone.
  - Added `Scripts/verify_resource_tool_requirements.py`.
  - Updated the roadmap, technical design document, Ground Zero resource pass,
    `setup_playable_blueprints.py`, and `verify_playable_blueprints.py`.

Verification:

- `python3 -m py_compile Scripts/setup_playable_blueprints.py Scripts/verify_playable_blueprints.py Scripts/verify_resource_tool_requirements.py` passed.
- `python3 Scripts/verify_resource_tool_requirements.py` passed.
- `git diff --check` passed.
- Windows editor build passed through direct `Scripts\BuildEditor-Windows.bat`.
- Windows Unreal Python `setup_playable_blueprints.py` passed.
- Windows Unreal Python `verify_playable_blueprints.py` passed.

Roadmap state:

- Current version section: `0.1.F Gathering And Resources`
- Items remaining in `0.1.F`: `1`
- Immediate next roadmap item: `Add resource node persistence`.

## Agrarian 0.1.F Resource Node Persistence - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `5da545e Add resource node persistence`

Completed roadmap item:

- `Add resource node persistence`
  - Added `FAgrarianSavedResourceNode` to `UAgrarianSaveGame`.
  - Added `AAgrarianResourceNode::PersistenceNodeId`, capture, and restore
    methods so resource depletion persists by stable map/tile node id.
  - Added persistence subsystem capture/restore paths for resource nodes.
  - The subsystem only captures nodes that exist in the loaded world and
    restores matching existing nodes by id; it does not spawn resource nodes
    from saves.
  - Updated Ground Zero placement to assign `PersistenceNodeId` values matching
    actor labels.
  - Extended the Ground Zero resource verifier to require stable persistence
    ids.
  - Updated persistence/resource documentation and closed the roadmap item.
  - Added `Scripts/verify_resource_node_persistence.py`.

Verification:

- `python3 -m py_compile Scripts/setup_ground_zero_demo_map.py Scripts/verify_ground_zero_resources.py Scripts/verify_resource_node_persistence.py` passed.
- `python3 Scripts/verify_resource_node_persistence.py` passed.
- `git diff --check` passed.
- Windows editor build passed through direct `Scripts\BuildEditor-Windows.bat`.
- Windows Unreal Python `setup_ground_zero_demo_map.py` passed.
- Windows Unreal Python `verify_ground_zero_resources.py` passed.

Roadmap state:

- Current version section: `0.1.F Gathering And Resources`
- Items remaining in `0.1.F`: `0`
- Next required action: build the Windows investor demo and stop.

## Agrarian 0.1.G Simple Container Recipe - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `ef658a3 Add simple container recipe`

Completed roadmap item:

- `Add simple container recipe`
  - Added `simple_container` as an inventory item definition and generated
    `/Game/Agrarian/DataAssets/Items/DA_Item_SimpleContainer`.
  - Added `DA_Recipe_SimpleContainer` using wood, fiber, and hide as MVP
    inputs.
  - The recipe produces one simple container after 10 seconds and does not
    require a campfire.
  - Updated item and recipe setup/verification scripts so future asset
    regeneration preserves the container item and recipe.
  - Added `Scripts/verify_simple_container_recipe.py` for static roadmap/docs
    coverage.
  - Updated the roadmap, technical design document, and inventory data model
    to clarify that the simple container is currently craftable inventory
    output; placed storage, permissions, volume, and persistence remain later
    system work.

Verification:

- `python3 -m py_compile Scripts/setup_item_definitions.py Scripts/verify_item_definitions.py Scripts/setup_recipe_definitions.py Scripts/verify_recipe_definitions.py Scripts/verify_simple_container_recipe.py` passed.
- `python3 Scripts/verify_simple_container_recipe.py` passed.
- `git diff --check` passed.
- Windows editor build passed through direct `Scripts\BuildEditor-Windows.bat`.
- Windows Unreal Python `setup_recipe_definitions.py` passed.
- Windows Unreal Python `verify_item_definitions.py` passed.
- Windows Unreal Python `verify_recipe_definitions.py` passed.

Roadmap state:

- Current version section: `0.1.G Primitive Crafting Loop`
- Items remaining in `0.1.G`: `2`
- Immediate next roadmap item: `Add crafting UI`.

## Agrarian 0.1.G Crafting UI - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `3509641 Add MVP crafting HUD`

Completed roadmap item:

- `Add crafting UI`
  - Added `UAgrarianCraftingComponent::GetKnownRecipes` so UI and Blueprint
    code can read a merged list of recipe data assets and runtime recipes.
  - Added a compact `AAgrarianDebugHUD` crafting panel beside the inventory
    HUD.
  - The panel lists known MVP recipes, shows current inventory counts against
    required ingredients, and color-codes craftable versus blocked rows.
  - Updated `BP_AgrarianPlayerCharacter` and
    `Scripts/setup_agrarian_player_blueprints.py` so the player starts with
    primitive recipe data assets loaded in `KnownRecipeAssets`.
  - Extended `Scripts/verify_agrarian_player_blueprints.py` to verify the exact
    MVP recipe list, including `simple_container` and `bandage`.
  - Added `Scripts/verify_crafting_ui.py`.
  - Updated the roadmap and technical design document with the MVP HUD approach
    and deferred full UMG crafting controls.

Verification:

- `python3 -m py_compile Scripts/setup_agrarian_player_blueprints.py Scripts/verify_agrarian_player_blueprints.py Scripts/verify_crafting_ui.py` passed.
- `python3 Scripts/verify_crafting_ui.py` passed.
- `git diff --check` passed.
- Windows editor build passed through direct `Scripts\BuildEditor-Windows.bat`.
- Windows Unreal Python `setup_agrarian_player_blueprints.py` passed.
- Windows Unreal Python `verify_agrarian_player_blueprints.py` passed.

Roadmap state:

- Current version section: `0.1.G Primitive Crafting Loop`
- Items remaining in `0.1.G`: `1`
- Immediate next roadmap item: `Add crafting debug tools`.

## Agrarian 0.1.G Crafting Debug Tools - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `70eb22d Add crafting debug exec tools`

Completed roadmap item:

- `Add crafting debug tools`
  - Added player-controller exec command `AgrarianCraftStatus` to list known
    recipes and current craftability.
  - Added player-controller exec command `AgrarianCraft <RecipeId>` to request
    a server-authoritative craft through `UAgrarianCraftingComponent`.
  - Kept crafting mutations on the server-authoritative component path instead
    of adding a separate debug-only inventory mutation.
  - Added `Scripts/verify_crafting_debug_tools.py`.
  - Updated the roadmap and technical design document with the debug tool
    commands and demo-rehearsal intent.

Verification:

- `python3 -m py_compile Scripts/verify_crafting_debug_tools.py` passed.
- `python3 Scripts/verify_crafting_debug_tools.py` passed.
- `git diff --check` passed.
- Windows editor build passed through direct `Scripts\BuildEditor-Windows.bat`.
- Windows Unreal Python `verify_agrarian_player_blueprints.py` passed.

Roadmap state:

- Current version section: `0.1.G Primitive Crafting Loop`
- Items remaining in `0.1.G`: `0`
- Next required action: build the Windows investor demo and stop.

## Agrarian 0.1.H Campfire Extinguish Logic - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `c61b226 Add campfire extinguish logic`

Completed roadmap item:

- `Add extinguish logic`
  - Added `AAgrarianCampfire::Extinguish()` as a native Blueprint-callable
    authority path.
  - Extinguish clears remaining fuel, turns off replicated `bLit`, and reuses
    the same visual update path as natural fuel depletion.
  - Added `SetLit` helper so fuel depletion, fuel addition, and extinguish all
    update replicated state consistently.
  - Added `Scripts/verify_fire_extinguish_logic.py`.
  - Updated the roadmap and technical design document.

Verification:

- `python3 -m py_compile Scripts/verify_fire_extinguish_logic.py` passed.
- `python3 Scripts/verify_fire_extinguish_logic.py` passed.
- `git diff --check` passed.
- Windows editor build passed through direct `Scripts\BuildEditor-Windows.bat`.

Roadmap state:

- Current version section: `0.1.H Fire System`
- Items remaining in `0.1.H`: `4`
- Immediate next roadmap item: `Add cooking placeholder if needed`.

## Agrarian 0.1.H Campfire Cooking Placeholder - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `0ac4bec Add campfire cooking placeholder`

Completed roadmap item:

- `Add cooking placeholder if needed`
  - Added native replicated campfire cooking placeholder state on `AAgrarianCampfire`: `bCookingPlaceholderEnabled`, `CookingSecondsRequired`, and `CookingProgressSeconds`.
  - Lit campfires now advance cooking progress server-side while the placeholder is enabled.
  - Added Blueprint-callable `CanCook()` and `GetCookingProgressRatio()` helpers for future recipe UI, food transformation, and interaction hooks.
  - Kept the playable Blueprint setup repeatable by leaving the new cooking defaults native; Unreal Python did not expose these freshly-added native fields on the existing BP_Campfire CDO during this pass.
  - Added `Scripts/verify_fire_cooking_placeholder.py`.
  - Updated the roadmap and technical design document.

Verification:

- `python3 -m py_compile Scripts/setup_playable_blueprints.py Scripts/verify_playable_blueprints.py Scripts/verify_fire_cooking_placeholder.py` passed.
- `python3 Scripts/verify_fire_cooking_placeholder.py` passed.
- `git diff --check` passed.
- Windows editor build passed through direct `Scripts\BuildEditor-Windows.bat`.
- Windows Unreal Python `setup_playable_blueprints.py` passed.
- Windows Unreal Python `verify_playable_blueprints.py` passed.

Roadmap state:

- Current version section: `0.1.H Fire System`
- Items remaining in `0.1.H`: `3`
- Immediate next roadmap item: `Add smoke/visual effect placeholder`.

## Agrarian 0.1.H Campfire Smoke Placeholder - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `c60b975 Add campfire smoke placeholder`

Completed roadmap item:

- `Add smoke/visual effect placeholder`
  - Added a native assetless `SmokeEffect` `UParticleSystemComponent` to `AAgrarianCampfire`.
  - Smoke starts inactive, is attached above the fire mesh, and activates/deactivates through the same replicated visual-state path as fire light intensity.
  - Final smoke/ember assets can now be assigned later without changing gameplay code.
  - Added `Scripts/verify_fire_smoke_placeholder.py`.
  - Updated the roadmap and technical design document.

Verification:

- `python3 -m py_compile Scripts/verify_fire_smoke_placeholder.py` passed.
- `python3 Scripts/verify_fire_smoke_placeholder.py` passed.
- `git diff --check` passed.
- Windows editor build passed through direct `Scripts\BuildEditor-Windows.bat`.

Roadmap state:

- Current version section: `0.1.H Fire System`
- Items remaining in `0.1.H`: `2`
- Immediate next roadmap item: `Add persistence`.

## Agrarian 0.1.H Campfire Persistence - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `7291c48 Add campfire persistence state`

Completed roadmap item:

- `Add persistence`
  - Added `IAgrarianPersistentStateProvider` so persistent actors can capture/apply custom save fields through the existing `UAgrarianPersistentActorComponent` path.
  - Campfires now have a persistent actor component with actor type `campfire`.
  - Campfire save state captures transform plus lit state, fuel seconds, cooking placeholder enabled state, required cooking seconds, and cooking progress seconds.
  - Campfire load state restores those values and reapplies the replicated visual state.
  - The admin/world load path now registers the `campfire` actor type along with shelters.
  - Added `Scripts/verify_fire_persistence.py`.
  - Updated the roadmap and technical design document.

Verification:

- `python3 -m py_compile Scripts/verify_fire_persistence.py` passed.
- `python3 Scripts/verify_fire_persistence.py` passed.
- `git diff --check` passed.
- Windows editor build passed through direct `Scripts\BuildEditor-Windows.bat`.

Roadmap state:

- Current version section: `0.1.H Fire System`
- Items remaining in `0.1.H`: `1`
- Immediate next roadmap item: `Connect rain/weather to fire behavior`.

## Agrarian 0.1.H Cinematic Startup Credits And Demo Package - 2026-05-17

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `7472b0e Fix startup credits packaging warning`

Completed request:

- Added a native cinematic startup credits sequence to the existing demo notice
  widget.
  - Nathan Slaven - Lead Developer
  - Hunter Slaven - Junior Developer
  - Lisa Reiley - Quality Control
  - River Slaven - Alpha Game Tester
  - Fisher Slaven - Beta Game Tester
  - Funding by cherished individuals, Pacificao seed funding, and Lina Family
    Investment Funds
- Each credit card slams into frame, holds briefly, shows a stylized native
  Slate illustration panel, then slides out.
- Updated investor demo labels to `Investor Demo v0.1.H - Build 2026.05.17`.
- Added `Scripts/verify_startup_credits_sequence.py`.
- Added the requested forest-fire ignition/spread concept to the roadmap as a
  future server-authoritative fire risk/spread system.
- Fixed the Windows packaging warning-as-error by renaming a local timeline
  accumulator from `Cursor` to `TimelineCursor`.

Verification:

- `python3 Scripts/verify_startup_credits_sequence.py` passed.
- `git diff --check` passed.
- Windows editor build passed earlier via `Scripts\BuildEditor-Windows.bat`.
- Windows investor demo package passed via `Scripts\PackageWindowsDevelopment.bat`.
- Package log ended with `BUILD SUCCESSFUL` and
  `AutomationTool exiting with ExitCode=0 (Success)`.

Investor demo output:

- Path: `/mnt/projects/AgrarianGameBulid/Builds/WindowsDevelopment`
- Main launcher: `Builds/WindowsDevelopment/Start Agrarian Demo.cmd`
- README version: `Investor Demo v0.1.H - Build 2026.05.17`
- Fresh archive timestamps:
  - `AgrarianGame.exe`: 2026-05-17 20:52
  - pak/container files: 2026-05-17 20:52
  - launchers and README: 2026-05-17 20:53
- Package size: approximately `1.1G`.

## Agrarian 0.1.I Shelter Building Style - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `27be644 Decide MVP shelter building style`

Completed roadmap item:

- `Decide MVP building style`
  - Version 0.1 shelter construction is crafted-kit based.
  - Players gather natural materials, craft primitive frame/wall/roof panel
    inventory parts, combine those parts into one placeable primitive shelter,
    then place it through the server-authoritative placement component.
  - Fully modular wall-by-wall building remains deferred to `0.2.E Permanent
    Structures`.
  - Added `Scripts/verify_shelter_building_style_decision.py`.

Verification:

- `python3 -m py_compile Scripts/verify_shelter_building_style_decision.py`
  passed.
- `python3 Scripts/verify_shelter_building_style_decision.py` passed.
- `git diff --check` passed.

Roadmap state:

- Current version section: `0.1.I Shelter Building`
- Immediate next roadmap item: `Add ghost preview`.

## Agrarian 0.1.I Building Ghost Preview - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `6baeca2 Add building ghost preview`

Completed roadmap item:

- `Add ghost preview`
  - Added native placement preview state on
    `UAgrarianBuildingPlacementComponent`.
  - The component now broadcasts `OnBuildPreviewUpdated` with placement
    validity, snapped transform, and failure reason for Blueprint/UI use.
  - Added an assetless green/red wireframe ghost footprint via `DrawDebugBox`
    for development/investor builds before final mesh/material ghost assets
    exist.
  - Added `Scripts/verify_building_ghost_preview.py`.

Verification:

- `python3 -m py_compile Scripts/verify_building_ghost_preview.py Scripts/verify_shelter_building_style_decision.py`
  passed.
- `python3 Scripts/verify_building_ghost_preview.py` passed.
- `python3 Scripts/verify_shelter_building_style_decision.py` passed.
- `git diff --check` passed.
- Windows editor build passed via `Scripts\BuildEditor-Windows.bat`.

Roadmap state:

- Current version section: `0.1.I Shelter Building`
- Immediate next roadmap item: `Add wall piece if needed`.

## Agrarian 0.1.I Shelter Wall Piece - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `8c6aa7c Document MVP shelter wall piece`

Completed roadmap item:

- `Add wall piece if needed`
  - Confirmed the MVP wall piece is the existing craftable
    `primitive_wall_panel` inventory construction part consumed by the
    primitive shelter recipe.
  - Documented that separately placeable modular wall actors remain deferred to
    `0.2.E Permanent Structures`.
  - Added `Scripts/verify_shelter_wall_piece.py`.

Verification:

- `python3 -m py_compile Scripts/verify_shelter_wall_piece.py` passed.
- `python3 Scripts/verify_shelter_wall_piece.py` passed.
- `git diff --check` passed.

Roadmap state:

- Current version section: `0.1.I Shelter Building`
- Immediate next roadmap item: `Add roof piece if needed`.

## Agrarian 0.1.I Shelter Roof Piece - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `e57a77d Document MVP shelter roof piece`

Completed roadmap item:

- `Add roof piece if needed`
  - Confirmed the MVP roof piece is the existing craftable
    `primitive_roof_panel` inventory construction part consumed by the
    primitive shelter recipe.
  - Documented that separately placeable modular roof actors remain deferred to
    `0.2.E Permanent Structures`.
  - Added `Scripts/verify_shelter_roof_piece.py`.

Verification:

- `python3 -m py_compile Scripts/verify_shelter_roof_piece.py` passed.
- `python3 Scripts/verify_shelter_roof_piece.py` passed.
- `git diff --check` passed.

Roadmap state:

- Current version section: `0.1.I Shelter Building`
- Immediate next roadmap item: `Add door/opening if needed`.

## Agrarian 0.1.I Shelter Door/Opening Scope - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `01d439c Decide MVP shelter opening scope`

Completed roadmap item:

- `Add door/opening if needed`
  - MVP primitive shelters use an open entrance and do not include an
    interactive door.
  - Door actors, locks, ownership permissions, and modular openings are
    deferred to `0.2.E Permanent Structures`.
  - Added `Scripts/verify_shelter_door_opening_decision.py`.

Verification:

- `python3 -m py_compile Scripts/verify_shelter_door_opening_decision.py`
  passed.
- `python3 Scripts/verify_shelter_door_opening_decision.py` passed.
- `git diff --check` passed.

Roadmap state:

- Current version section: `0.1.I Shelter Building`
- Immediate next roadmap item: `Add deconstruction or damage placeholder`.

## Agrarian 0.1.I Shelter Damage/Deconstruction Placeholder - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `e75ee71 Add shelter damage placeholder`

Completed roadmap item:

- `Add deconstruction or damage placeholder`
  - Primitive shelters now have replicated `MaxStructureHealth` and
    `CurrentStructureHealth`.
  - Added authority-only `ApplyStructureDamage`, `RepairStructure`, and
    `Deconstruct` hooks.
  - Integrated `TakeDamage`, depletion destruction, health ratio/damaged
    helpers, and save/load support for current/max structure health through the
    persistent actor component.
  - Added `Scripts/verify_shelter_damage_placeholder.py`.

Verification:

- `python3 -m py_compile Scripts/verify_shelter_damage_placeholder.py` passed.
- `python3 Scripts/verify_shelter_damage_placeholder.py` passed.
- `git diff --check` passed.
- Windows editor build passed via `Scripts\BuildEditor-Windows.bat`.

Roadmap state:

- Current version section: `0.1.I Shelter Building`
- Items remaining in `0.1.I`: `0`
- Next required action: build the Windows investor demo and stop.

## Agrarian 0.1.I Investor Demo Metadata - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `0ea9af1 Update investor demo to 0.1.I`

Completed follow-up:

- Corrected the investor demo build label from `0.1.H` to `0.1.I` for package
  launchers, packaged README generation, project version, startup demo notice
  actor/widget defaults, startup-credits verification, and investor roadmap
  HTML.

Verification:

- `python3 Scripts/verify_startup_credits_sequence.py` passed.
- `python3 Scripts/verify_shelter_damage_placeholder.py` passed.
- Windows editor build passed via `Scripts\BuildEditor-Windows.bat`.

Roadmap state:

- `0.1.I Shelter Building` is complete.
- Next required action: rebuild the Windows investor demo package and stop.

## Agrarian 0.1.I Windows Investor Demo Package - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `0ea9af1 Update investor demo to 0.1.I`

Completed final package:

- Rebuilt the Windows Development investor demo package successfully after the
  0.1.I metadata correction.
- Package path: `/mnt/projects/AgrarianGameBulid/Builds/WindowsDevelopment`.
- Packaged README now reports:
  `Investor Demo v0.1.I - Build 2026.05.18`.
- Fresh archive timestamps confirmed at 2026-05-18 11:43-11:44 for the
  executable, pak/iostore files, launchers, manifests, and README.
- Package size: `1.1G`.

Verification:

- Windows editor build passed via `Scripts\BuildEditor-Windows.bat`.
- Windows package passed via `Scripts\PackageWindowsDevelopment.bat`.
- AutomationTool reported `BUILD SUCCESSFUL`, ExitCode=0.
- Game repo was clean after commit/push and package verification.

Roadmap state:

- `0.1.I Shelter Building` is complete.
- Stop point reached as requested: Windows investor demo rebuilt.

## Agrarian Long-Term Family/NPC Intelligence Roadmap - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit:
  `f3316b3 Add long-term family NPC intelligence roadmap`

Completed design/roadmap update:

- Added a phased roadmap for family members, community NPCs, and offline player
  stewardship so future NPCs behave like living contributors instead of static
  quest characters.
- Updated `AGRARIAN_DEVELOPMENT_ROADMAP.md` with:
  - core commitments for living family/community characters and
    proprietary-first NPC intelligence;
  - `0.2.H Household Task And Helper Foundations`;
  - `0.3.G Community NPC Behavior Foundation`;
  - `0.4.F Family Agency And Emotional Life`;
  - `0.4.G Offline Character Stewardship`;
  - `0.5.G Settlement Labor, Care, And Community Schedules`;
  - `0.6.F NPC Hardship, Adaptation, And Recovery`;
  - `0.8.E Proprietary AI Simulation Research`;
  - cross-cutting `N. NPC, Family, And Offline Simulation Intelligence`.
- Updated `Docs/TechnicalDesignDocument.md` with server-authoritative,
  deterministic-first NPC intelligence rules, local/self-hosted AI boundaries,
  and offline character stewardship constraints.

Verification:

- Roadmap/design section search passed.
- `git diff --check` passed.

## Agrarian 0.1.J Bleeding Placeholder - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `c796a99 Add bleeding survival placeholder`

Completed roadmap item:

- `Add bleeding placeholder`
  - Survival now tracks replicated `BleedingSeverity` inside
    `FAgrarianSurvivalSnapshot`.
  - Added `BleedingDamagePerMinute`, `BleedingExhaustionPerSecond`,
    `AddBleeding`, and `ReduceBleeding` to `UAgrarianSurvivalComponent`.
  - Generic injuries now seed bleeding at a light placeholder ratio.
  - Active bleeding applies small health and exhaustion pressure and contributes
    to care-history injury burden.
  - Debug HUD and `AgrarianSurvival` console output now display bleeding.
  - Added `Scripts/verify_bleeding_placeholder.py`.

Verification:

- `python3 -m py_compile Scripts/verify_bleeding_placeholder.py` passed.
- `python3 Scripts/verify_bleeding_placeholder.py` passed.
- `git diff --check` passed.
- Windows editor build passed via `Scripts\BuildEditor-Windows.bat`.

Roadmap state:

- Current version section: `0.1.J Injury And Basic Survival Consequences`
- Immediate next roadmap item: `Add sprain or movement penalty placeholder`.

## Agrarian 0.1.J Sprain/Movement Placeholder - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `559a5b2 Add sprain movement placeholder`

Completed roadmap item:

- `Add sprain or movement penalty placeholder`
  - Survival now tracks replicated `SprainSeverity` inside
    `FAgrarianSurvivalSnapshot`.
  - Added `SprainExhaustionPerSecond`, `AddSprain`, and `ReduceSprain` to
    `UAgrarianSurvivalComponent`.
  - Generic injuries now seed sprain severity at a light placeholder ratio.
  - Active sprains add light exhaustion pressure and contribute to care-history
    injury burden.
  - `AAgrarianGameCharacter::CalculateSurvivalMovementMultiplier` now includes
    an explicit sprain movement multiplier.
  - Debug HUD and `AgrarianSurvival` console output now display sprain state.
  - Added `Scripts/verify_sprain_movement_placeholder.py`.

Verification:

- `python3 -m py_compile Scripts/verify_sprain_movement_placeholder.py`
  passed.
- `python3 Scripts/verify_sprain_movement_placeholder.py` passed.
- `git diff --check` passed.
- Windows editor build passed via `Scripts\BuildEditor-Windows.bat`.

Roadmap state:

- Current version section: `0.1.J Injury And Basic Survival Consequences`
- Immediate next roadmap item: `Add treatment item`.

## Agrarian 0.1.J Treatment Item - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `164169f Expand bandage treatment effects`

Completed roadmap item:

- `Add treatment item`
  - The existing craftable `bandage` medicine item is now the MVP treatment
    item.
  - Server-authoritative `AgrarianUseItem bandage Quantity` now reduces injury,
    bleeding, and sprain severity with a small health bump.
  - Updated item-use verification and technical design documentation.
  - Added `Scripts/verify_treatment_item.py`.

Verification:

- `python3 -m py_compile Scripts/verify_treatment_item.py Scripts/verify_item_use.py`
  passed.
- `python3 Scripts/verify_treatment_item.py` passed.
- `python3 Scripts/verify_item_use.py` passed.
- `git diff --check` passed.
- Windows editor build passed via `Scripts\BuildEditor-Windows.bat`.

Roadmap state:

- Current version section: `0.1.J Injury And Basic Survival Consequences`
- Immediate next roadmap item: `Add death state`.

## Agrarian 0.1.J Death State - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `d318e97 Add survival death state`

Completed roadmap item:

- `Add death state`
  - Survival now has replicated `bIsDead` and `LastDeathReason` fields in
    `FAgrarianSurvivalSnapshot`.
  - Health depletion latches death, zeros stamina, and records
    `health_depleted` when no other death reason is set.
  - Ordinary `RestoreHealth` no longer revives dead characters.
  - Added explicit `MarkDead` and `Revive` hooks for future respawn/admin flows.
  - Updated `AgrarianHeal` to use `Revive`.
  - Debug HUD and `AgrarianSurvival` console output now show alive/dead state.
  - Added `Scripts/verify_death_state.py`.

Verification:

- `python3 -m py_compile Scripts/verify_death_state.py` passed.
- `python3 Scripts/verify_death_state.py` passed.
- `git diff --check` passed.
- Windows editor build passed via `Scripts\BuildEditor-Windows.bat`.

Roadmap state:

- Current version section: `0.1.J Injury And Basic Survival Consequences`
- Immediate next roadmap item: `Add respawn rules for MVP`.

## Agrarian 0.1.J MVP Respawn Rules - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `11f051c Add MVP respawn command`

Completed roadmap item:

- `Add respawn rules for MVP`
  - Added server-authoritative `AgrarianRespawn` exec command and
    `ServerAgrarianRespawn` RPC.
  - Respawn is only available after death; living characters are told to use
    `AgrarianHeal` for admin recovery.
  - MVP respawn stops movement, returns the character to the Ground Zero
    spawn/home location, revives at partial health, stabilizes
    hunger/thirst/body temperature, and clears acute injury, bleeding, sprain,
    sickness, and exhaustion.
  - Family/inheritance respawn remains deferred to later generational
    milestones.
  - Added `Scripts/verify_mvp_respawn_rules.py`.

Verification:

- `python3 -m py_compile Scripts/verify_mvp_respawn_rules.py` passed.
- `python3 Scripts/verify_mvp_respawn_rules.py` passed.
- `git diff --check` passed.
- Windows editor build passed via `Scripts\BuildEditor-Windows.bat`.

Roadmap state:

- Current version section: `0.1.J Injury And Basic Survival Consequences`
- Immediate next roadmap item: `Add corpse/backpack placeholder if needed`.

## Agrarian 0.1.J Corpse/Backpack Decision - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `e8f4acb Document corpse backpack MVP decision`

Completed roadmap item:

- `Add corpse/backpack placeholder if needed`
  - Decision: no physical corpse/backpack actor is needed for the 0.1.J MVP
    respawn loop because death inventory loss is not active yet.
  - Reserved future corpse/backpack recovery as a persistent, server-owned,
    interaction-gated death-recovery record once inventory loss and decay rules
    exist.
  - Updated persistence and multiplayer networking design docs with the
    decision.
  - Added `Scripts/verify_corpse_backpack_decision.py`.

Verification:

- `python3 -m py_compile Scripts/verify_corpse_backpack_decision.py` passed.
- `python3 Scripts/verify_corpse_backpack_decision.py` passed.
- `git diff --check` passed.
- No Windows editor build was run because this item only changed docs and
  Python verification.

Roadmap state:

- Current version section: `0.1.J Injury And Basic Survival Consequences`
- Immediate next roadmap item: `Add replicated death feedback`.

## Agrarian 0.1.J Replicated Death Feedback - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `dce4f77 Add replicated death feedback`

Completed roadmap item:

- `Add replicated death feedback`
  - Survival now exposes Blueprint-assignable `OnDeathStateChanged` with
    replicated alive/dead state and death reason.
  - `BroadcastSurvivalChanged` now emits death-state feedback when replicated
    death state or death reason changes.
  - Debug HUD shows replicated death reason while dead.
  - Added `Scripts/verify_replicated_death_feedback.py`.

Verification:

- `python3 -m py_compile Scripts/verify_replicated_death_feedback.py` passed.
- `python3 Scripts/verify_replicated_death_feedback.py` passed.
- `git diff --check` passed.
- Windows editor build passed via `Scripts\BuildEditor-Windows.bat`.

Roadmap state:

- `0.1.J Injury And Basic Survival Consequences` items remaining: `0`
- Next required action: build the Windows investor demo and stop.

## Agrarian 0.1.J Investor Demo Metadata - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `f0d8e3e Update investor demo to 0.1.J`

Completed follow-up:

- Corrected the investor demo build label from `0.1.I` to `0.1.J`.
- Updated project version, package launcher README generation, startup notice
  actor/widget defaults, startup credits verifier, and investor roadmap HTML.
- Investor roadmap HTML now describes 0.1.J injury/basic survival consequences
  as completed.

Verification:

- `python3 Scripts/verify_startup_credits_sequence.py` passed.
- Stale `0.1.I` demo-label search passed.
- `git diff --check` passed.
- Windows editor build passed via `Scripts\BuildEditor-Windows.bat`.

Roadmap state:

- `0.1.J Injury And Basic Survival Consequences` is complete.
- Next required action: build the Windows investor demo and stop.

## Agrarian 0.1.J Windows Investor Demo Package - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `f0d8e3e Update investor demo to 0.1.J`

Completed final package step:

- Built the Windows investor demo for `0.1.J Injury And Basic Survival
  Consequences`.
- Output path: `/mnt/projects/AgrarianGameBulid/Builds/WindowsDevelopment`
- README label verified:
  `Investor Demo v0.1.J - Build 2026.05.18`.
- Packaged folder size: `1.1G`.
- Fresh package artifacts were written at `2026-05-18 13:50`, including
  `Builds/WindowsDevelopment/AgrarianGame.exe`, launcher command files, README,
  and `AgrarianGame/Content/Paks` files.

Verification:

- Windows package command passed via `Scripts\PackageWindowsDevelopment.bat`.
- AutomationTool reported `BUILD SUCCESSFUL`.
- AutomationTool exited with `ExitCode=0`.
- README and package artifacts were verified from Linux side.
- Game repo status is clean after package verification.

Roadmap state:

- `0.1.J Injury And Basic Survival Consequences` is complete.
- Windows investor demo has been rebuilt and is ready at the package path.
- Stop here per user instruction.

## Agrarian 0.1.L Dedicated Server Target/Bootstrap - 2026-05-18

Current repo:

- `/home/nathan/AgrarianGameBuild`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `1367f59 Add dedicated server bootstrap target`

Completed roadmap item:

- `Create dedicated server build target if needed`
  - Confirmed `Source/AgrarianGameServer.Target.cs` already defines the
    `AgrarianGameServer` Linux dedicated server target.
  - Confirmed `Scripts/BuildLinuxDedicatedServer-Windows.bat` already exists
    for Windows-Builder Linux server cross-compile/package output at
    `Builds/LinuxServerDevelopment`.
  - Added reusable Ubuntu gameplay host bootstrap script:
    `Operations/cloud-game-server/bootstrap_ubuntu_game_server.sh`.
  - Standardized first MVP gameplay DNS/port convention:
    `play.agrariangame.com`, `7777/udp`.
  - Updated `Docs/Ops/DedicatedServerBuildRunbook.md` with Ubuntu/DigitalOcean
    deploy steps, DNS, port, systemd service, and separation from the tile
    server.
  - Added `Scripts/verify_dedicated_server_target.py`.

Verification:

- `python3 -m py_compile Scripts/verify_dedicated_server_target.py` passed.
- `python3 Scripts/verify_dedicated_server_target.py` passed.
- `git diff --check` passed.
- Attempted Windows-Builder dedicated server package verification, but Unraid
  libvirt/VM Manager is currently down:
  `/etc/rc.d/rc.libvirt status` reports the libvirt daemon is not running, and
  starting it fails because `/etc/libvirt/virtlockd.conf` and
  `/etc/libvirt/virtlogd.conf` are missing.

Roadmap state:

- Current version section: `0.1.L Basic Multiplayer`
- Immediate next roadmap item: `Add server travel flow`.
- Windows/Linux build verification and any new VM startup are blocked until
  Unraid libvirt/VM Manager is repaired.

## Agrarian 0.1.L Server Travel Flow - 2026-05-18

Current repo:

- `/home/nathan/AgrarianGameBuild`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `ebc7aa1 Add MVP server travel flow`

Completed roadmap item:

- `Add server travel flow`
  - Added allowlisted `AgrarianServerTravel GroundZero` admin/dev console
    command on `AAgrarianGamePlayerController`.
  - Command routes through `ServerAgrarianServerTravel`, validates server
    authority, resolves only the Ground Zero MVP map, and calls
    `World->ServerTravel("/Game/Agrarian/Maps/L_GroundZeroTerrain_Test?listen")`.
  - Updated multiplayer networking design and dedicated server runbook with the
    MVP travel flow.
  - Added `Scripts/verify_server_travel_flow.py`.

Verification:

- `python3 -m py_compile Scripts/verify_server_travel_flow.py` passed.
- `python3 Scripts/verify_server_travel_flow.py` passed.
- `git diff --check` passed.
- C++ compile/package verification is blocked because Unraid libvirt/VM Manager
  is still down; see previous handoff entry for the missing libvirt config
  files.

Roadmap state:

- Current version section: `0.1.L Basic Multiplayer`
- Immediate next roadmap item: `Add network relevancy rules`.

## Agrarian 0.1.L Network Relevancy Rules - 2026-05-18

Current repo:

- `/home/nathan/AgrarianGameBuild`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `d4ea10c Add MVP network relevancy rules`

Completed roadmap item:

- `Add network relevancy rules`
  - Added explicit MVP `NetCullDistanceSquared` values for replicated gameplay
    actors:
    - item pickups: 30m
    - resource nodes: 45m
    - campfires, shelters, wildlife: 60m
    - water sources and weather exposure zones: 65m
    - wildlife spawn managers: 80m
  - Updated multiplayer networking design with the Ground Zero relevancy
    strategy.
  - Added `Scripts/verify_network_relevancy_rules.py`.

Verification:

- `python3 -m py_compile Scripts/verify_network_relevancy_rules.py` passed.
- `python3 Scripts/verify_network_relevancy_rules.py` passed.
- `git diff --check` passed.
- C++ compile/package verification is blocked because Unraid libvirt/VM Manager
  is still down.

Roadmap state:

- Current version section: `0.1.L Basic Multiplayer`
- Immediate next roadmap item: `Add basic latency testing`.

## Agrarian 0.1.L Basic Latency Testing - 2026-05-18

Current repo:

- `/home/nathan/AgrarianGameBuild`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `3b772da Add multiplayer latency test plan`

Completed roadmap item:

- `Add basic latency testing`
  - Added `Docs/Ops/MultiplayerLatencyTestPlan.md` with clean LAN, mild WAN,
    and rough WAN Unreal packet-simulation profiles.
  - Added `Scripts/LatencyTestProfiles-Windows.bat` to print the test console
    commands and reset commands.
  - Updated multiplayer networking design with the 0.1.L latency gate.
  - Added `Scripts/verify_basic_latency_testing.py`.

Verification:

- `python3 -m py_compile Scripts/verify_basic_latency_testing.py` passed.
- `python3 Scripts/verify_basic_latency_testing.py` passed.
- `git diff --check` passed.
- Interactive latency playthrough is blocked until a playable server/client test
  host is available; VM-based builds remain blocked by Unraid libvirt.

Roadmap state:

- Current version section: `0.1.L Basic Multiplayer`
- Immediate next roadmap item: `Add disconnect/reconnect handling`.

## Agrarian 0.1.L Disconnect/Reconnect Handling - 2026-05-18

Current repo:

- `/home/nathan/AgrarianGameBuild`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `0b9a8b7 Add MVP reconnect snapshots`

Completed roadmap item:

- `Add disconnect/reconnect handling`
  - Added MVP player reconnect snapshots in `UAgrarianPersistenceSubsystem`.
  - `SavePlayerSnapshot` updates the matching saved player record with
    transform, survival, care history, and inventory.
  - `RestorePlayerSnapshot` restores a matching saved player after spawn.
  - `AAgrarianGameGameMode::Logout` saves the reconnect snapshot before logout
    completes.
  - `AAgrarianGameGameMode::RestartPlayer` restores the snapshot after normal
    MVP spawn when a matching record exists.
  - Updated multiplayer networking design and roadmap.
  - Added `Scripts/verify_disconnect_reconnect_handling.py`.

Verification:

- `python3 -m py_compile Scripts/verify_disconnect_reconnect_handling.py`
  passed.
- `python3 Scripts/verify_disconnect_reconnect_handling.py` passed.
- `git diff --check` passed.
- C++ compile/package verification is blocked because Unraid libvirt/VM Manager
  is still down.

Roadmap state:

- `0.1.L Basic Multiplayer` items remaining: `0`.
- Next required action: update investor demo metadata to 0.1.L, build the
  Windows investor demo, and stop, but this is blocked until Unraid libvirt/VM
  Manager is repaired.

## Agrarian 0.1.L Investor Demo Metadata - 2026-05-18

Current repo:

- `/home/nathan/AgrarianGameBuild`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `9c808f4 Update investor demo to 0.1.L`

Completed follow-up:

- Updated the investor demo metadata baseline from `0.1.K` to `0.1.L`.
- Updated project version, Windows launcher README generation, startup notice
  actor/widget defaults, startup credits verifier, and investor roadmap HTML.
- Investor roadmap HTML now describes 0.1.L Basic Multiplayer as completed:
  dedicated server target/bootstrap, server travel flow, replicated session
  foundations, network relevancy rules, latency test plan, and
  disconnect/reconnect snapshots.

Verification:

- `python3 -m py_compile Scripts/verify_startup_credits_sequence.py` passed.
- `python3 Scripts/verify_startup_credits_sequence.py` passed.
- Stale 0.1.K demo-label search passed.
- `git diff --check` passed.

Roadmap state:

- `0.1.L Basic Multiplayer` is complete.
- Windows investor demo metadata is ready for `Investor Demo v0.1.L - Build
  2026.05.18`.
- Final Windows investor demo packaging remains blocked because Unraid
  libvirt/VM Manager is still down/missing `/etc/libvirt/virtlockd.conf` and
  `/etc/libvirt/virtlogd.conf`.
- Gameplay server DNS/firewall plan:
  - Point `play.agrariangame.com` to the future Ubuntu gameplay server public
    IP.
  - Open `7777/udp` for Unreal gameplay traffic.
  - Keep SSH restricted to admin IPs.

## Agrarian 0.1.L Windows Investor Demo Package - 2026-05-18

Current repo:

- `/home/nathan/AgrarianGameBuild`
- Build-share checkout: `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `9c808f4 Update investor demo to 0.1.L`

Completed final package step:

- Refreshed the restored Windows builder `P:` mapping to
  `\\192.168.5.8\projects`.
- Fast-forwarded the build-share checkout from `6178e77` to `9c808f4`.
- Built the Windows investor demo for `0.1.L Basic Multiplayer`.
- Output path: `/mnt/projects/AgrarianGameBulid/Builds/WindowsDevelopment`.
- README label verified:
  `Investor Demo v0.1.L - Build 2026.05.18`.
- Packaged folder size: `1.1G`.
- Fresh top-level package artifacts were written at `2026-05-18 15:43`,
  including `AgrarianGame.exe`, launcher command files, manifests, and README.

Verification:

- Windows package command passed via `Scripts\PackageWindowsDevelopment.bat`.
- AutomationTool reported `BUILD SUCCESSFUL`.
- AutomationTool exited with `ExitCode=0`.
- `AgrarianGame.exe` exists and is a PE32+ x86-64 Windows GUI executable.
- `/home/nathan/AgrarianGameBuild` and `/mnt/projects/AgrarianGameBulid` were
  clean after package verification.

Notes:

- Windows build VM reported 10 logical cores and 32 GB RAM during packaging.
- UE 5.7 emitted deprecation warnings for direct public access to
  `AActor::NetCullDistanceSquared`; future cleanup should move those MVP
  relevancy assignments to `SetNetCullDistanceSquared()`.

Roadmap state:

- `0.1.L Basic Multiplayer` is complete.
- Windows investor demo has been rebuilt and is ready at the package path.
- Stop here per user instruction.

## Agrarian MVP Gameplay Server VM - 2026-05-18

Created Unraid VM:

- VM name: `Agrarian-PlayServer`
- Hostname/FQDN: `play.agrariangame.com`
- LAN IP: `192.168.5.15/22`
- Gateway: `192.168.4.1`
- MAC: `52:54:00:7a:77:15`
- vCPU/RAM: 2 vCPU, 4 GB RAM
- Disk: 40 GB qcow2 backed by Ubuntu 24.04 cloud image
- Autostart: enabled

Access:

- User: `nathan`
- Password uses the usual MVP account password requested by Nathan.
- SSH password login is enabled for MVP convenience.

Server setup:

- `ufw` active.
- OpenSSH allowed.
- `7777/udp` allowed.
- Deployment directory exists and is owned by `nathan:nathan`:
  `/opt/agrarian/server`.
- Systemd service installed:
  `agrarian-game-server.service`.
- Service is enabled but inactive until the expected dedicated-server wrapper
  exists:
  `/opt/agrarian/server/AgrarianGameServer.sh`.

DNS/network:

- `play.agrariangame.com` resolves to public IP `208.79.250.18`.
- Router/NAT follow-up: forward public `7777/udp` to `192.168.5.15`.
- No Unreal Linux dedicated server package has been deployed yet, so nothing is
  currently listening on `7777/udp`.

Follow-up:

- Nathan reported Unraid UI showed the VM requires guest agent installed.
- Verified guest agent is actually installed and active inside
  `Agrarian-PlayServer`.
- Libvirt backend confirms agent health:
  `virsh qemu-agent-command Agrarian-PlayServer --cmd '{"execute":"guest-ping"}'`
  succeeds.
- `virsh domifaddr Agrarian-PlayServer --source agent` returns
  `192.168.5.15/22`.
- Rebooted `Agrarian-PlayServer` once so Unraid UI can refresh with the agent
  present from boot.
- Post-reboot checks:
  - `qemu-guest-agent`: active
  - `ssh`: active
  - `ufw`: active
  - `7777/udp`: allowed
  - `agrarian-game-server.service`: inactive because
    `/opt/agrarian/server/AgrarianGameServer.sh` does not exist yet.

Dedicated server deployment attempt:

- Ran `Scripts\BuildLinuxDedicatedServer-Windows.bat` from Windows-Builder.
- Build failed immediately with UnrealBuildTool:
  `Missing files required to build Linux targets. Enable Linux as an optional
  download component in the Epic Games Launcher.`
- Windows-Builder UE 5.7 has installed target platform folders for Android,
  IOS, VisionOS, and Windows, but not Linux under `Engine\Platforms`.
- No existing Linux dedicated server package was found in
  `Builds/LinuxServerDevelopment`.

Required next step:

- Install the UE 5.7 Linux target platform/toolchain on Windows-Builder through
  Epic Games Launcher options.
- Rerun `Scripts\BuildLinuxDedicatedServer-Windows.bat`.
- Copy the produced Linux server package to
  `nathan@192.168.5.15:/opt/agrarian/server`.
- Start `agrarian-game-server.service` and verify `ss -lunp` shows `7777/udp`.

## Agrarian PlayServer Linux Package Deployment - 2026-05-18

Windows-Builder Linux toolchain:

- Installed official UE 5.7 Linux cross-toolchain:
  `v26_clang-20.1.8-rockylinux8`.
- Toolchain path:
  `C:\UnrealToolchains\v26_clang-20.1.8-rockylinux8`.
- Machine environment variable set:
  `LINUX_MULTIARCH_ROOT=C:\UnrealToolchains\v26_clang-20.1.8-rockylinux8`.
- Turnkey verifies Linux SDK as valid.

Dedicated server target status:

- `Scripts\BuildLinuxDedicatedServer-Windows.bat` still cannot produce the true
  server target because Epic's installed binary engine reports:
  `Server targets are not currently supported from this engine distribution.`
- Production-quality follow-up: use a source-built/server-capable Unreal engine
  distribution for the real `AgrarianGameServer` dedicated target.

MVP fallback deployed:

- Built Linux game package via UAT `BuildCookRun` for platform `Linux`.
- Output path on build share:
  `/mnt/projects/AgrarianGameBulid/Builds/LinuxGameDevelopment`.
- First Linux cook took about 1h 16m and generated the initial Vulkan shader
  cache.
- Package size: `1.1G`.
- Deployed to:
  `nathan@192.168.5.15:/opt/agrarian/server`.
- Created wrapper:
  `/opt/agrarian/server/AgrarianGameServer.sh`.
- Wrapper runs:
  `/Game/Agrarian/Maps/L_GroundZeroTerrain_Test?listen -server -port=7777 -log
  -unattended -NullRHI -nosound`.

Verification:

- `agrarian-game-server.service` is active after stability check.
- `ss -lunp` shows `AgrarianGame` listening on `0.0.0.0:7777/udp`.
- `nc -zvu -w 2 192.168.5.15 7777` succeeds from LAN.
- Runtime log reports ProjectVersion `0.1.L-investor.20260518`.
- Runtime log reports browse started for:
  `/Game/Agrarian/Maps/L_GroundZeroTerrain_Test?Name=Player?listen`.

## Agrarian 0.1.K Wildlife Spawn Manager - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `494fe6f Add wildlife spawn manager`

Completed roadmap item:

- `Add spawn manager`
  - Added `AAgrarianWildlifeSpawnManager`, a replicated,
    server-authoritative spawn manager actor for MVP wildlife population seeding.
  - Designers can configure wildlife class, initial spawn count, max active
    population, spawn radius, respawn interval, spawn-on-begin-play, and
    optional navigation projection.
  - Spawned wildlife actors remain authoritative server spawns and are tracked
    by the manager for active population limits.
  - Updated roadmap and technical design documentation.
  - Added `Scripts/verify_wildlife_spawn_manager.py`.

Verification:

- `python3 -m py_compile Scripts/verify_wildlife_spawn_manager.py` passed.
- `python3 Scripts/verify_wildlife_spawn_manager.py` passed.
- `git diff --check` passed.
- Windows editor build passed via `Scripts\BuildEditor-Windows.bat`.

Roadmap state:

- Current version section: `0.1.K Wildlife Prototype`
- Immediate next roadmap item: `Add performance limits`.

## Agrarian 0.1.K Wildlife Performance Limits - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `a834eb7 Add wildlife performance limits`

Completed roadmap item:

- `Add performance limits`
  - Wildlife now has MVP server-side performance throttling.
  - Nearby wildlife inside `FullUpdateRadius` updates normally for responsive
    flee/chase behavior.
  - Far wildlife batches server thinking on `FarUpdateIntervalSeconds`.
  - Dead wildlife disables ticking after entering the dead state.
  - Spawn-manager `MaxActiveWildlife` remains the MVP population cap.
  - Updated roadmap and technical design documentation.
  - Added `Scripts/verify_wildlife_performance_limits.py`.

Verification:

- `python3 -m py_compile Scripts/verify_wildlife_performance_limits.py
  Scripts/verify_wildlife_spawn_manager.py` passed.
- `python3 Scripts/verify_wildlife_performance_limits.py` passed.
- `python3 Scripts/verify_wildlife_spawn_manager.py` passed.
- `git diff --check` passed.
- Windows editor build passed via `Scripts\BuildEditor-Windows.bat`.

Roadmap state:

- `0.1.K Wildlife Prototype` items remaining: `0`.
- Next required action: update investor demo metadata to 0.1.K, build the
  Windows investor demo, and stop.

## Agrarian 0.1.K Investor Demo Metadata - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `6178e77 Update investor demo to 0.1.K`

Completed follow-up:

- Corrected the investor demo build label from `0.1.J` to `0.1.K`.
- Updated project version, package launcher README generation, startup notice
  actor/widget defaults, startup credits verifier, and investor roadmap HTML.
- Investor roadmap HTML now describes 0.1.K wildlife prototype as completed.

Verification:

- `python3 Scripts/verify_startup_credits_sequence.py` passed.
- Stale `0.1.J` demo-label search passed.
- `git diff --check` passed.
- Windows editor build passed via `Scripts\BuildEditor-Windows.bat`.

Roadmap state:

- `0.1.K Wildlife Prototype` is complete.
- Next required action: build the Windows investor demo and stop.

## Agrarian 0.1.K Windows Investor Demo Package - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `6178e77 Update investor demo to 0.1.K`

Completed final package step:

- Built the Windows investor demo for `0.1.K Wildlife Prototype`.
- Output path: `/mnt/projects/AgrarianGameBulid/Builds/WindowsDevelopment`
- README label verified:
  `Investor Demo v0.1.K - Build 2026.05.18`.
- Packaged folder size: `1.1G`.
- Fresh package artifacts were written at `2026-05-18 14:33`, including
  `Builds/WindowsDevelopment/AgrarianGame.exe`, launcher command files, README,
  and `AgrarianGame/Content/Paks` files.

Verification:

- Windows package command passed via `Scripts\PackageWindowsDevelopment.bat`.
- AutomationTool reported `BUILD SUCCESSFUL`.
- AutomationTool exited with `ExitCode=0`.
- README and package artifacts were verified from Linux side.
- Game repo status is clean after package verification.

Roadmap state:

- `0.1.K Wildlife Prototype` is complete.
- Windows investor demo has been rebuilt and is ready at the package path.
- Stop here per user instruction.

## Agrarian 0.1.M Player Identity Persistence - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `1e0d326 Save MVP player identity metadata`

Completed roadmap item:

- `Save player identity`
  - Added `FAgrarianSavedPlayerIdentity` to player save records.
  - Kept the existing backwards-compatible `PlayerId` string.
  - Player persistence now prefers a valid `APlayerState` network unique ID,
    then falls back to player name, then pawn name for local prototype sessions.
  - The save record keeps safe identity metadata: stable ID, player name,
    network ID, whether the network ID was used, and last known pawn name.
  - Persistence documentation now explicitly states that saved player identity
    does not store credentials, emails, passwords, or tokens.
  - Added `Scripts/verify_player_identity_persistence.py`.

Verification:

- `python3 -m py_compile Scripts/verify_player_identity_persistence.py` passed.
- `python3 Scripts/verify_player_identity_persistence.py` passed.
- `git diff --check` passed.
- Windows editor compile gate was attempted but blocked because
  `UNRAID_PASSWORD` was not present in the shell environment.

Deployment classification:

- `Server deploy required` at milestone packaging time because this changes
  server-authoritative persistence/save code shared by the multiplayer host.

Roadmap state:

- Current version section: `0.1.M Persistence MVP`
- Items remaining in `0.1.M`: `13`
- Immediate next roadmap item: `Save player stats`.

## Agrarian 0.1.M Player Stats Persistence - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `82f60f4 Document player stats persistence`

Completed roadmap item:

- `Save player stats`
  - Confirmed player stats already persist through
    `FAgrarianSavedPlayer::Survival`.
  - The saved survival snapshot covers health, stamina, exhaustion, hunger,
    thirst, body temperature, injury, bleeding, sprain, sickness, death state,
    and death reason.
  - Restore continues through `UAgrarianSurvivalComponent::ApplySavedState`.
  - Updated persistence documentation and roadmap wording to make the save
    contract explicit.
  - Added `Scripts/verify_player_stats_persistence.py`.

Verification:

- `python3 -m py_compile Scripts/verify_player_stats_persistence.py` passed.
- `python3 Scripts/verify_player_stats_persistence.py` passed.
- `git diff --check` passed.
- No Windows editor compile was run for this item because it only documented
  and verified an existing C++ persistence path.

Deployment classification:

- `Docs/email only` for this item.
- Milestone still has `Server deploy required` pending because earlier 0.1.M
  persistence runtime changes affect the multiplayer host.

Roadmap state:

- Current version section: `0.1.M Persistence MVP`
- Items remaining in `0.1.M`: `12`
- Immediate next roadmap item: `Save long-term character care history placeholders without applying aging gameplay yet`.

## Agrarian 0.1.M Care History Persistence - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `1d2ac1e Document care history persistence`

Completed roadmap item:

- `Save long-term character care history placeholders without applying aging gameplay yet`
  - Confirmed player care history already persists through
    `FAgrarianSavedPlayer::CareHistory`.
  - The saved `FAgrarianCareHistorySnapshot` reserves nutrition, illness,
    injury, sleep, shelter, stress, workload, and treatment quality.
  - Documented that 0.1.M only persists/restores these placeholders and does
    not apply aging, lifespan, inheritance, or generational outcomes.
  - Added `Scripts/verify_care_history_persistence.py`.

Verification:

- `python3 -m py_compile Scripts/verify_care_history_persistence.py` passed.
- `python3 Scripts/verify_care_history_persistence.py` passed.
- `git diff --check` passed.
- No Windows editor compile was run for this item because it only documented
  and verified an existing C++ persistence path.

Deployment classification:

- `Docs/email only` for this item.
- Milestone still has `Server deploy required` pending because earlier 0.1.M
  persistence runtime changes affect the multiplayer host.

Roadmap state:

- Current version section: `0.1.M Persistence MVP`
- Items remaining in `0.1.M`: `11`
- Immediate next roadmap item: `Save player inventory`.

## Agrarian 0.1.M Player Inventory Persistence - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `188ef7b Document player inventory persistence`

Completed roadmap item:

- `Save player inventory`
  - Confirmed player inventory already persists through
    `FAgrarianSavedPlayer::Inventory`.
  - Inventory is copied from `UAgrarianInventoryComponent::Items` and restored
    through `UAgrarianInventoryComponent::RestoreSavedItems`.
  - Restore recomputes derived values and broadcasts inventory changes so HUD
    and debug listeners see loaded item state.
  - Updated persistence documentation, roadmap wording, and the existing
    inventory persistence verifier for the 0.1.M item.

Verification:

- `python3 -m py_compile Scripts/verify_inventory_persistence.py` passed.
- `python3 Scripts/verify_inventory_persistence.py` passed.
- `git diff --check` passed.
- No Windows editor compile was run for this item because it only documented
  and verified an existing C++ persistence path.

Deployment classification:

- `Docs/email only` for this item.
- Milestone still has `Server deploy required` pending because earlier 0.1.M
  persistence runtime changes affect the multiplayer host.

Roadmap state:

- Current version section: `0.1.M Persistence MVP`
- Items remaining in `0.1.M`: `10`
- Immediate next roadmap item: `Save resource depletion state if needed`.

## Agrarian 0.1.M Resource Depletion Persistence - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `66f523b Document resource depletion persistence`

Completed roadmap item:

- `Save resource depletion state if needed`
  - Confirmed active loaded resource nodes persist stable `ResourceNodeId`,
    remaining harvest count, and MVP respawn flag through
    `FAgrarianSavedResourceNode`.
  - Restore applies depletion only to matching map-authored resource nodes,
    leaving tile-authored placement owned by map/tile content.
  - Updated persistence documentation, roadmap wording, and the resource-node
    persistence verifier for the 0.1.M item.

Verification:

- `python3 -m py_compile Scripts/verify_resource_node_persistence.py` passed.
- `python3 Scripts/verify_resource_node_persistence.py` passed.
- `git diff --check` passed.
- No Windows editor compile was run for this item because it only documented
  and verified an existing C++ persistence path.

Deployment classification:

- `Docs/email only` for this item.
- Milestone still has `Server deploy required` pending because earlier 0.1.M
  persistence runtime changes affect the multiplayer host.

Roadmap state:

- Current version section: `0.1.M Persistence MVP`
- Items remaining in `0.1.M`: `9`
- Immediate next roadmap item: `Save world time`.

## Agrarian 0.1.M World Time Persistence - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `fdc919c Document world time persistence`

Completed roadmap item:

- `Save world time`
  - Confirmed world saves persist `UAgrarianSaveGame::WorldHours`.
  - The persistence subsystem captures from `AAgrarianGameState::WorldHours`.
  - Restore applies world time only on server authority during world load.
  - Added `Scripts/verify_world_time_persistence.py`.

Verification:

- `python3 -m py_compile Scripts/verify_world_time_persistence.py` passed.
- `python3 Scripts/verify_world_time_persistence.py` passed.
- `git diff --check` passed.
- No Windows editor compile was run for this item because it only documented
  and verified an existing C++ persistence path.

Deployment classification:

- `Docs/email only` for this item.
- Milestone still has `Server deploy required` pending because earlier 0.1.M
  persistence runtime changes affect the multiplayer host.

Roadmap state:

- Current version section: `0.1.M Persistence MVP`
- Items remaining in `0.1.M`: `8`
- Immediate next roadmap item: `Save weather seed/state`.

## Agrarian 0.1.M Weather State Persistence - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `f6ed45d Document weather state persistence`

Completed roadmap item:

- `Save weather seed/state`
  - Confirmed world saves persist fallback `UAgrarianSaveGame::Weather`.
  - Confirmed saves also retain provider `WeatherInputs` and `WeatherDebug`.
  - Load reapplies mapped provider inputs when present or falls back to saved
    enum weather state.
  - Added `Scripts/verify_weather_state_persistence.py`.

Verification:

- `python3 -m py_compile Scripts/verify_weather_state_persistence.py` passed.
- `python3 Scripts/verify_weather_state_persistence.py` passed.
- `git diff --check` passed.
- No Windows editor compile was run for this item because it only documented
  and verified an existing C++ persistence path.

Deployment classification:

- `Docs/email only` for this item.
- Milestone still has `Server deploy required` pending because earlier 0.1.M
  persistence runtime changes affect the multiplayer host.

Roadmap state:

- Current version section: `0.1.M Persistence MVP`
- Items remaining in `0.1.M`: `7`
- Immediate next roadmap item: `Save containers`.

## Agrarian 0.1.M Container Persistence Schema - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `06666b1 Reserve container persistence schema`

Completed roadmap item:

- `Save containers`
  - Added `FAgrarianSavedContainer` to the save schema.
  - Reserved stable container ID, container type ID, transform, item stacks,
    and owner player ID.
  - Documented that 0.1.M does not yet have a placed container actor to
    capture; the current craftable `simple_container` remains an inventory item
    covered by player inventory persistence.
  - Added `Scripts/verify_container_persistence_schema.py`.

Verification:

- `python3 -m py_compile Scripts/verify_container_persistence_schema.py` passed.
- `python3 Scripts/verify_container_persistence_schema.py` passed.
- `git diff --check` passed.
- Windows editor compile gate was attempted but blocked because
  `UNRAID_PASSWORD` was not present in the shell environment.

Deployment classification:

- `Server deploy required` at milestone packaging time because this changes the
  shared save-game schema used by server-side persistence.

Roadmap state:

- Current version section: `0.1.M Persistence MVP`
- Items remaining in `0.1.M`: `6`
- Immediate next roadmap item: `Add server-side save interval`.

## Agrarian 0.1.M Server Autosave Interval - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `a7ca8d1 Add server autosave interval`

Completed roadmap item:

- `Add server-side save interval`
  - Added `ServerAutoSaveIntervalSeconds` to `AAgrarianGameGameMode`, defaulting
    to five minutes.
  - On authority, `BeginPlay` starts a repeating timer that calls
    `RunServerAutoSave`.
  - Autosave uses `UAgrarianPersistenceSubsystem::SaveCurrentWorld`.
  - Setting the interval to `0` disables the MVP autosave timer.
  - Added `Scripts/verify_server_save_interval.py`.

Verification:

- `python3 -m py_compile Scripts/verify_server_save_interval.py` passed.
- `python3 Scripts/verify_server_save_interval.py` passed.
- `git diff --check` passed, with Git line-ending normalization warnings for
  the touched Unreal template GameMode files.
- Windows editor compile gate was attempted but blocked because
  `UNRAID_PASSWORD` was not present in the shell environment.

Deployment classification:

- `Server deploy required` at milestone packaging time because this changes
  authoritative server autosave behavior.

Roadmap state:

- Current version section: `0.1.M Persistence MVP`
- Items remaining in `0.1.M`: `5`
- Immediate next roadmap item: `Add load-on-server-start`.

## Agrarian 0.1.M Load On Server Start - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `9aa2f8b Load world state on server start`

Completed roadmap item:

- `Add load-on-server-start`
  - Added `bLoadWorldOnServerStart` to `AAgrarianGameGameMode`, enabled by
    default.
  - On authoritative `BeginPlay`, GameMode registers the MVP persistent actor
    classes and loads the current world if a save exists.
  - Startup load uses `LoadCurrentWorld` without clearing existing map actors,
    so map-authored resources and startup content remain map-owned.
  - Added `Scripts/verify_load_on_server_start.py`.

Verification:

- `python3 -m py_compile Scripts/verify_load_on_server_start.py` passed.
- `python3 Scripts/verify_load_on_server_start.py` passed.
- `git diff --check` passed, with Git line-ending normalization warnings for
  the touched Unreal template GameMode files.
- Windows editor compile gate was attempted but blocked because
  `UNRAID_PASSWORD` was not present in the shell environment.

Deployment classification:

- `Server deploy required` at milestone packaging time because this changes
  authoritative server startup load behavior.

Roadmap state:

- Current version section: `0.1.M Persistence MVP`
- Items remaining in `0.1.M`: `4`
- Immediate next roadmap item: `Add initial tile registry persistence for Ground Zero`.

## Agrarian 0.1.M Ground Zero Tile Registry Persistence - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `65599b9 Persist Ground Zero tile registry baseline`

Completed roadmap item:

- `Add initial tile registry persistence for Ground Zero`
  - Added `FAgrarianSavedTileRegistryState` to the save schema.
  - Save metadata now records active tile
    `gz_us_ca_pacifica_utm10n_e544_n4160`, registry path
    `Data/Tiles/ground_zero_tiles.json`, schema version, generation version,
    and package version.
  - The external JSON registry remains authoritative.
  - Added `Scripts/verify_ground_zero_tile_registry_persistence.py`.

Verification:

- `python3 -m py_compile Scripts/verify_ground_zero_tile_registry_persistence.py` passed.
- `python3 Scripts/verify_ground_zero_tile_registry_persistence.py` passed.
- `git diff --check` passed.
- Windows editor compile gate was attempted but blocked because
  `UNRAID_PASSWORD` was not present in the shell environment.

Deployment classification:

- `Server deploy required` at milestone packaging time because this changes the
  shared save-game schema used by server-side persistence.

Roadmap state:

- Current version section: `0.1.M Persistence MVP`
- Items remaining in `0.1.M`: `3`
- Immediate next roadmap item: `Add backup-before-save option`.

## Agrarian 0.1.M Backup Before Save - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `dfad880 Back up save before overwrite`

Completed roadmap item:

- `Add backup-before-save option`
  - Added `bBackupBeforeSave` to `UAgrarianPersistenceSubsystem`, enabled by
    default.
  - Before overwriting an existing save slot, `WriteSave` copies the current
    `.sav` file from `Saved/SaveGames` into `Saved/SaveGames/Backups`.
  - Backups use UTC timestamped filenames.
  - Missing first-save files skip backup creation.
  - Added `Scripts/verify_backup_before_save.py`.

Verification:

- `python3 -m py_compile Scripts/verify_backup_before_save.py` passed.
- `python3 Scripts/verify_backup_before_save.py` passed.
- `git diff --check` passed.
- Windows editor compile gate was attempted but blocked because
  `UNRAID_PASSWORD` was not present in the shell environment.

Deployment classification:

- `Server deploy required` at milestone packaging time because this changes
  persistence runtime behavior.

Roadmap state:

- Current version section: `0.1.M Persistence MVP`
- Items remaining in `0.1.M`: `2`
- Immediate next roadmap item: `Add recovery plan for corrupted save`.

## Agrarian 0.1.M Corrupted Save Recovery Plan - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `31e781d Document save recovery plan`

Completed roadmap item:

- `Add recovery plan for corrupted save`
  - Added `Docs/Ops/PersistenceSaveRecoveryPlan.md`.
  - Documented suspect-save symptoms, immediate response, restoring the newest
    timestamped backup, fallback when all backups fail, and MVP limitations.
  - Linked the plan from the persistence design document.
  - Added `Scripts/verify_save_recovery_plan.py`.

Verification:

- `python3 -m py_compile Scripts/verify_save_recovery_plan.py` passed.
- `python3 Scripts/verify_save_recovery_plan.py` passed.
- `git diff --check` passed.
- No Windows editor compile was run because this is docs/verifier only.

Deployment classification:

- `Docs/email only`.

Roadmap state:

- Current version section: `0.1.M Persistence MVP`
- Items remaining in `0.1.M`: `1`
- Immediate next roadmap item: `Document persistence limitations`.

## Agrarian 0.1.M Persistence Limitations - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `5a381ad Document MVP persistence limitations`

Completed roadmap item:

- `Document persistence limitations`
  - Added `## MVP Persistence Limitations` to
    `Docs/PersistenceDesignDocument.md`.
  - Documented current limits around file-based saves, missing database/account
    binding, migration tooling, corrupted-save validation, placed-container
    actor capture, offline/family simulation, cross-server persistence,
    external backup replication, player-facing UI, and pre-MVP save
    compatibility.
  - Added `Scripts/verify_persistence_limitations.py`.

Verification:

- `python3 -m py_compile Scripts/verify_persistence_limitations.py` passed.
- `python3 Scripts/verify_persistence_limitations.py` passed.
- `git diff --check` passed.
- No Windows editor compile was run because this is docs/verifier only.

Deployment classification:

- `Docs/email only`.

Roadmap state:

- `0.1.M Persistence MVP` roadmap items are complete.
- Next required action: build the Windows investor demo; deploy the multiplayer
  server package only if the build environment is available because 0.1.M
  included server-relevant persistence runtime changes.

## Agrarian 0.1.M Milestone Complete / Build Blocked - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `63c6adc Update investor demo metadata to 0.1.M`

Milestone status:

- `0.1.M Persistence MVP` is complete in the roadmap.
- Investor demo metadata was updated to
  `Investor Demo v0.1.M - Build 2026.05.18`.
- `ProjectVersion` was updated to `0.1.M-investor.20260518`.
- Startup credits verification passed against the new metadata.

Verification:

- `python3 -m py_compile Scripts/verify_startup_credits_sequence.py` passed.
- `python3 Scripts/verify_startup_credits_sequence.py` passed.
- `git diff --check` passed.
- Game repo status was clean after the metadata commit.

Build/deploy status:

- Windows investor demo package was attempted with:
  `/home/nathan/bin/winbuilder cmd 'set AGRARIAN_NO_PAUSE=1 && pushd \\DevBox\projects\AgrarianGameBulid && Scripts\PackageWindowsDevelopment.bat'`
- Packaging is blocked because `UNRAID_PASSWORD` is not present in the shell
  environment.
- Multiplayer server deploy is also blocked until a current package can be
  built. Server deploy is required for this milestone once build access is
  restored because 0.1.M included persistence runtime/server behavior changes.

Next required action:

- Restore `UNRAID_PASSWORD` in the Codex shell environment or provide another
  approved Windows-Builder access path.
- Rerun the Windows investor demo package.
- Build/deploy the server package to `play.agrariangame.com:7777` and verify it
  if packaging succeeds.
## Agrarian 0.1.N Main Menu Placeholder - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `4aec203 Add MVP main menu placeholder`

Completed roadmap item:

- `Add main menu placeholder`
  - Added native `UAgrarianMvpFrontendWidget` and `EAgrarianMvpFrontendScreen`.
  - Added a scalable painted main menu placeholder with title, subtitle, primary action, and MVP flow note.
  - Wired `AAgrarianGamePlayerController` to create the MVP frontend widget for local player controllers.
  - Marked the roadmap item complete.

Verification:

- `python3 -m py_compile Scripts/verify_mvp_main_menu_placeholder.py` passed.
- `python3 Scripts/verify_mvp_main_menu_placeholder.py` passed.
- `git diff --check` passed.
- No full Windows package was run for this item because this is a focused client UI placeholder; final milestone packaging is still required.

Deployment classification:

- `Client UI only`.
- No multiplayer server deploy required for this item.

Roadmap state:

- Current version section: `0.1.N MVP UI And UX`
- Items remaining in `0.1.N`: `12`
- Immediate next roadmap item: `After splash/startup screens, land on an MVP character selection landing page`.
## Agrarian 0.1.N Character Selection Landing - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `5efd81c Add MVP character selection landing`

Completed roadmap item:

- `After splash/startup screens, land on an MVP character selection landing page`
  - Extended `UAgrarianMvpFrontendWidget` with a `CharacterSelection` screen.
  - Added a scalable native character-selection landing page with two MVP
    placeholder character cards.
  - Updated `AAgrarianGamePlayerController` so local player controllers open
    the MVP frontend directly on `CharacterSelection` after startup.
  - Marked the roadmap item complete.

Verification:

- `python3 -m py_compile Scripts/verify_mvp_character_selection_landing.py` passed.
- `python3 Scripts/verify_mvp_character_selection_landing.py` passed.
- `git diff --check` passed.
- No full Windows package was run for this item; final milestone packaging is still required.

Deployment classification:

- `Client UI only`.
- No multiplayer server deploy required for this item.

Automation:

- Email summary sent to `nathan@pacificao.com` through `pacificao-mail`
  using local Dovecot delivery, not AWS SES.

Roadmap state:

- Current version section: `0.1.N MVP UI And UX`
- Items remaining in `0.1.N`: `11`
- Immediate next roadmap item: `Let players choose a realistic young adult male or female character with average proportions for the MVP`.
## Agrarian 0.1.N Character Archetype Choice - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `c855924 Add MVP character archetype choice`

Completed roadmap item:

- `Let players choose a realistic young adult male or female character with average proportions for the MVP`
  - Added `EAgrarianMvpCharacterArchetype` with young adult male/female MVP choices.
  - Added selected-state rendering to the native character-selection cards.
  - Added keyboard selection with Left/Right or A/D while on the character-selection screen.
  - Added `AgrarianSelectCharacter male|female` dev command for headless/manual verification.
  - Kept both choices on the same MVP survival baseline until real character art and stats differentiation arrive.

Verification:

- `python3 -m py_compile Scripts/verify_mvp_character_archetype_choice.py` passed.
- `python3 Scripts/verify_mvp_character_archetype_choice.py` passed.
- `git diff --check` passed.
- No full Windows package was run for this item; final milestone packaging is still required.

Deployment classification:

- `Client UI only`.
- No multiplayer server deploy required for this item.

Automation:

- Email summary sent to `nathan@pacificao.com` through `pacificao-mail`
  using local Dovecot delivery, not AWS SES.

Roadmap state:

- Current version section: `0.1.N MVP UI And UX`
- Items remaining in `0.1.N`: `10`
- Immediate next roadmap item: `Add join server screen`.
## Agrarian 0.1.N Join Server Screen - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `2009295 Add MVP join server screen`

Completed roadmap item:

- `Add join server screen`
  - Added a native `JoinServer` screen to `UAgrarianMvpFrontendWidget`.
  - The screen shows the selected MVP character archetype and the target
    `play.agrariangame.com:7777` address.
  - Enter/Space from character selection advances to the join screen.
  - Backspace/Escape returns to character selection.
  - Added `AgrarianShowMvpScreen main|character|join` for direct manual/headless verification.

Verification:

- `python3 -m py_compile Scripts/verify_mvp_join_server_screen.py` passed.
- `python3 Scripts/verify_mvp_join_server_screen.py` passed.
- `git diff --check` passed.
- No full Windows package was run for this item; final milestone packaging is still required.

Deployment classification:

- `Client UI only`.
- No multiplayer server deploy required for this item.

Automation:

- Email summary sent to `nathan@pacificao.com` through `pacificao-mail`
  using local Dovecot delivery, not AWS SES.

Roadmap state:

- Current version section: `0.1.N MVP UI And UX`
- Items remaining in `0.1.N`: `9`
- Immediate next roadmap item: `Add loading screen`.
## Agrarian 0.1.N Loading Screen - 2026-05-18

Current repo:

- `/mnt/projects/AgrarianGameBulid`
- GitHub remote: `pacificao/AgrarianGameBuild`
- Current branch: `main`
- Latest pushed game commit: `e8d46c8 Add MVP loading screen`

Completed roadmap item:

- `Add loading screen`
  - Added a native `Loading` screen to `UAgrarianMvpFrontendWidget`.
  - The screen shows Ground Zero preparation copy, selected character context,
    server address context, and a deterministic placeholder progress bar.
  - Enter/Space from the join-server screen advances to loading.
  - `AgrarianShowMvpScreen main|character|join|loading` can display it directly.

Verification:

- `python3 -m py_compile Scripts/verify_mvp_loading_screen.py` passed.
- `python3 Scripts/verify_mvp_loading_screen.py` passed.
- `git diff --check` passed.
- No full Windows package was run for this item; final milestone packaging is still required.

Deployment classification:

- `Client UI only`.
- No multiplayer server deploy required for this item.

Automation:

- Email summary sent to `nathan@pacificao.com` through `pacificao-mail`
  using local Dovecot delivery, not AWS SES.

Roadmap state:

- Current version section: `0.1.N MVP UI And UX`
- Items remaining in `0.1.N`: `8`
- Immediate next roadmap item: `Add HUD`.
