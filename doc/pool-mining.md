# Desktop pool mining preview

This is a mainnet desktop feature, not a network-rule change. It leaves the
existing solo CPU mining and staking controls separate. Do not run a staging
build against the production pool just to test its GUI.

In a package containing a compatible CPU miner, choose **Mining → Mine on
All The Acronyms**, set the CPU thread count, and click **Start pool mining**.
One thread is the default. The desktop wallet chooses a receiving address
that it controls, and passes only that public address and a worker suffix to
the fixed pool. It does not pass wallet secrets. Closing the pool window or
quitting Agrarian Core stops the local miner. Accepted shares are not payouts.

## Packaging and release gates

The preview does not download or install miners. It expects a separately
reviewed Quark-capable `cpuminer` or `cpuminer.exe` under `miners/` next to
`agrarian-qt`. The source release does not bundle that helper. Before a
one-click release, build it from reviewed, pinned source, include its license
and corresponding source with any distributed binary, and smoke-test the exact
Windows and Ubuntu packages. The [pool's current miner](https://github.com/tpruvot/cpuminer-multi)
is a starting point, not an approval of its existing upstream binaries.

Verify startup, disconnect/reconnect, accepted/rejected shares, clean shutdown
and payout credit on an isolated test pool. Use a disposable wallet, check it
owns the payout address, and verify that configured CPU thread counts and
resource usage match. Do not override operating system antivirus or execution
policy. A missing or blocked miner must fail with a visible explanation.

GPU mining is not included. It needs a compatible, independently tested
backend for the target GPU and operating system. GPU device selection and
intensity are separate from CPU thread count.
