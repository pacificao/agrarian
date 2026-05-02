Dependencies
============

These are the dependencies currently used by the Agrarian 2.0 modernization
branch. You can find instructions for installing or building them in the
`build-*.md` file for your platform. The preferred path is the deterministic
`depends/` tree, not mixed system libraries.

| Dependency | Version used | Minimum required | CVEs | Shared | Bundled Qt library |
| --- | --- | --- | --- | --- | --- |
| Berkeley DB | [4.8.30](https://www.oracle.com/technetwork/database/database-technologies/berkeleydb/downloads/index.html) | 4.8.x | No |  |  |
| Boost | [1.91.0](https://www.boost.org/users/download/) | [1.47.0](https://github.com/bitcoin/bitcoin/pull/8920) | No |  |  |
| Clang |  | C++17 support |  |  |  |
| D-Bus | [1.10.18](https://cgit.freedesktop.org/dbus/dbus/tree/NEWS?h=dbus-1.10) |  | No | Yes |  |
| Expat | [2.8.0](https://libexpat.github.io/) |  | Security fixes | Yes |  |
| fontconfig | [2.12.1](https://www.freedesktop.org/software/fontconfig/release/) |  | No | Yes |  |
| FreeType | [2.14.3](https://download.savannah.gnu.org/releases/freetype) |  | Qt 6 static font support |  |  |
| GCC |  | C++17 support |  |  |  |
| HarfBuzz-NG |  |  |  |  |  |
| libevent | [2.1.12-stable](https://github.com/libevent/libevent/releases) | 2.0.22 | No |  |  |
| libjpeg |  |  |  |  | [Yes](https://github.com/agrarian-project/agrarian/blob/master/depends/packages/qt.mk#L65) |
| libpng |  |  |  |  | [Yes](https://github.com/agrarian-project/agrarian/blob/master/depends/packages/qt.mk#L64) |
| librsvg | |  |  |  |  |
| MiniUPnPc | [2.0.20180203](http://miniupnp.free.fr/files) |  | No |  |  |
| OpenSSL | [3.5.6](https://www.openssl.org/source) |  | No |  |  |
| GMP | [6.1.2](https://gmplib.org/) | | No | | |
| PCRE |  |  |  |  | [Yes](https://github.com/agrarian-project/agrarian/blob/master/depends/packages/qt.mk#L66) |
| protobuf | [2.6.1](https://github.com/google/protobuf/releases) |  | No |  |  |
| Python (tests) |  | [3.5](https://www.python.org/downloads) |  |  |  |
| qrencode | [3.4.4](https://fukuchi.org/works/qrencode) |  | No |  |  |
| Qt | [6.8.3](https://download.qt.io/official_releases/qt/) | 6.8 LTS | No |  |  |
| XCB |  |  |  |  | [Yes](https://github.com/agrarian-project/agrarian/blob/master/depends/packages/qt.mk#L87) (Linux only) |
| xkbcommon |  |  |  |  | [Yes](https://github.com/agrarian-project/agrarian/blob/master/depends/packages/qt.mk#L86) (Linux only) |
| ZeroMQ | [4.3.1](https://github.com/zeromq/libzmq/releases) | 4.0.0 | No |  |  |
| zlib | [1.3.2](https://zlib.net/) |  |  |  | No |

Modernization notes
-------------------

* Qt builds target Qt 6.8 LTS through the deterministic depends path.
* OpenSSL builds target OpenSSL 3.5 LTS through the deterministic depends path.
* Boost builds target Boost 1.91.0 through the deterministic depends path.
* Berkeley DB remains 4.8.30 in depends for legacy wallet portability. Wallets
  built against other BDB major versions may not be portable.

Controlling dependencies
------------------------
Some dependencies are not needed in all configurations. The following are some factors that affect the dependency list.

#### Options passed to `./configure`
* MiniUPnPc is not needed with  `--with-miniupnpc=no`.
* Berkeley DB is not needed with `--disable-wallet`.
* Qt is not needed with `--without-gui`.
* If the qrencode dependency is absent, QR support won't be added. To force an error when that happens, pass `--with-qrencode`.
* ZeroMQ is needed only with the `--with-zmq` option.

#### Other
* librsvg is only needed if you need to run `make deploy` on (cross-compilation to) macOS.
