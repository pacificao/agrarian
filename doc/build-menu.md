Interactive Build Menu
======================

`contrib/agrarian-build-menu.sh` provides an interactive Ubuntu build workflow
for common Agrarian targets:

- Linux daemon and CLI tools
- Linux Qt GUI wallet
- Windows daemon and CLI tools
- Windows Qt GUI wallet

The script clones the Agrarian repository if the selected checkout directory
does not exist. If the checkout already exists, it fetches, checks out the
configured branch, and performs a fast-forward pull.

Quick Start
-----------

From an existing checkout:

    ./contrib/agrarian-build-menu.sh

From a fresh Ubuntu host:

    sudo apt-get update
    sudo apt-get install -y git ca-certificates
    git clone --branch 2.0 https://github.com/pacificao/agrarian.git agrarian
    cd agrarian
    ./contrib/agrarian-build-menu.sh

Defaults
--------

The script can be configured with environment variables:

    REPO_URL=https://github.com/pacificao/agrarian.git
    BRANCH=2.0
    WORKDIR=$HOME/agrarian
    JOBS=1

Example:

    JOBS=8 WORKDIR=$HOME/src/agrarian ./contrib/agrarian-build-menu.sh

Linux Daemon Autostart
----------------------

After a Linux daemon-capable build, the script asks whether to install and start
`agrariand` for the current user. If accepted, it creates:

- `$HOME/.local/bin/agrariand`
- `$HOME/.local/bin/agrarian-cli`
- `$HOME/.agrarian/agrarian.conf`
- `$HOME/.config/systemd/user/agrariand.service`

It then enables and starts the user service with:

    systemctl --user enable --now agrariand.service

Windows Build Output
--------------------

For Windows targets, the script prints the `.exe` artifact paths when the build
finishes. Copy the generated files from `src/` and `src/qt/` to the Windows
machine and run either `agrariand.exe` or `agrarian-qt.exe`.
