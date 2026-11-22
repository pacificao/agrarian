// Standalone test of CPU search and validation shared with future GPU workers.
#include "poolwork.h"

#include <cassert>
#include <cstdio>

template <typename Bytes> std::string Hex(const Bytes& bytes)
{
    static constexpr char alphabet[] = "0123456789abcdef";
    std::string result;
    for (const auto byte : bytes) {
        result += alphabet[byte >> 4];
        result += alphabet[byte & 15];
    }
    return result;
}

int main()
{
    poolwork::Header header{};
    const auto digest = poolwork::Digest(header, 0);
    const std::string expected = "633d8255a00e3a1ae1ee58d7d3a56387fb85f1068a6bb4ebf5a20315e57f0602";
    std::array<unsigned char, 32> digestBytes{};
    std::copy(digest.begin(), digest.end(), digestBytes.begin());
    assert(Hex(digestBytes) == expected);
    poolwork::Target equal{};
    std::copy(digest.begin(), digest.end(), equal.begin());
    assert(poolwork::Verify(header, equal, 0));
    auto below = equal;
    --below[0]; // same top 64 bits; the full target must reject it
    assert(!poolwork::Verify(header, below, 0));
    poolwork::Target all{};
    all.fill(0xff);
    std::atomic<bool> stop{false};
    auto last = poolwork::ScanCpu(header, all, poolwork::kNonceEnd - 1, 10, stop);
    assert(last.found && last.nonce == UINT32_MAX && last.nextNonce == poolwork::kNonceEnd && last.tested == 1);
    auto exhausted = poolwork::ScanCpu(header, all, last.nextNonce, 10, stop);
    assert(!exhausted.found && exhausted.tested == 0);
    auto empty = poolwork::ScanCpu(header, all, 10, 0, stop);
    assert(!empty.found && empty.tested == 0 && empty.nextNonce == 10);
    auto cap = poolwork::ScanCpu(header, poolwork::Target{}, 10, UINT32_MAX, stop);
    assert(!cap.found && cap.tested == poolwork::kMaxCpuBatch && cap.nextNonce == 10 + poolwork::kMaxCpuBatch);
    stop.store(true);
    auto stopped = poolwork::ScanCpu(header, all, 12, 10, stop);
    assert(!stopped.found && stopped.tested == 0 && stopped.nextNonce == 12);
    poolwork::Target previous{}, branch{};
    for (size_t i = 0; i < 32; ++i) {
        previous[i] = static_cast<unsigned char>(i);
        branch[i] = static_cast<unsigned char>(i + 32);
    }
    poolwork::Header work{};
    assert(poolwork::BuildHeader(previous, {1, 2, 3, 4}, {branch}, 5, 123456, 0x1e0ffff0, work));
    // Independent double-SHA256/struct.pack reference, including a Merkle branch.
    assert(Hex(work) == "0500000003020100070605040b0a09080f0e0d0c13121110171615141b1a19181f1e1d1c"
                        "f00439e4f7cd4d1a1511b226c5553fc390dc706a6144a6d0ea5553fcb5d7117c"
                        "40e20100f0ff0f1e00000000");
    const auto prior = work;
    assert(!poolwork::BuildHeader(previous, {}, {branch}, 5, 123456, 0x1e0ffff0, work));
    assert(!poolwork::BuildHeader(previous, std::vector<unsigned char>(poolwork::kMaxCoinbaseBytes + 1),
                                  {branch}, 5, 123456, 0x1e0ffff0, work));
    assert(!poolwork::BuildHeader(previous, {1}, std::vector<poolwork::Target>(poolwork::kMaxMerkleDepth + 1),
                                  5, 123456, 0x1e0ffff0, work));
    assert(work == prior);
    std::puts("First-party Quark work: header, digest, full target, bounds and cancellation passed.");
}
