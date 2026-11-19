# Agrarian Core

Agrarian Core is the open-source C++ node and desktop wallet for the Agrarian
(AGR) network. This GitHub repository is the official public home for source
code, issue tracking, contributions, and releases.

| Looking for | Start here |
| --- | --- |
| Project overview, whitepaper, explorer, web wallet | [AgrarianCoin.com](https://agrariancoin.com/) |
| Desktop wallet downloads | [GitHub Releases](https://github.com/pacificao/agrarian/releases) — no official Windows or Linux binaries published yet |
| Build from source | [Desktop wallet guide](doc/build-wallets.md) · [Ubuntu guide](doc/build-ubuntu-24.md) · [Windows cross-build guide](doc/build-windows.md) |
| Report an issue or contribute | [Issues](https://github.com/pacificao/agrarian/issues) · [Contributing](CONTRIBUTING.md) |
| Report a vulnerability | [Security policy](SECURITY.md) |

## Build and review

Start with the platform guide above for toolchain and Qt requirements. Clone the
official repository and review the build helper before running it:

```sh
git clone https://github.com/pacificao/agrarian.git
cd agrarian
```

The documented desktop targets are Ubuntu 24.04 x86-64 and Windows x86-64.
See the [release checklist](doc/desktop-wallet-release.md) for what must be
verified before a build is offered as an official download. The default branch
is development source, not a supported binary release.

- `src/`: core node, wallet, and Qt desktop interface
- `depends/`: pinned build dependencies
- `contrib/`: build, packaging, and smoke-test helpers
- `doc/`: developer and build documentation

Run `make check` in a build with tests enabled. Platform-specific smoke-test
instructions are in the [wallet build guide](doc/build-wallets.md).

## License

Agrarian Core is distributed under the [MIT license](COPYING).
