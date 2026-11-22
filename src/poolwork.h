// Distributed under the MIT software license, see COPYING.
#ifndef AGRARIAN_POOLWORK_H
#define AGRARIAN_POOLWORK_H

#include "hash.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <future>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <vector>

// Compute-only building block for the first-party desktop worker. Does not
// know about wallets, sockets, jobs from disk, or device drivers.
namespace poolwork {
using Header = std::array<unsigned char, 80>;
using Target = std::array<unsigned char, 32>; // canonical little-endian

constexpr uint64_t kNonceEnd = uint64_t{1} << 32; // exclusive
constexpr uint32_t kMaxCpuBatch = 4096;
constexpr size_t kMaxCoinbaseBytes = 4096;
constexpr size_t kMaxMerkleDepth = 32;

// Assemble the Quark header used by this pool. The previous block hash
// arrives with byte order reversed in each 32-bit word; the Merkle branch
// hashes and the computed root are already in wire byte order. Bits belongs
// in the header only; it must not replace the negotiated share target.
inline bool BuildHeader(const Target& stratumPrevious,
                        const std::vector<unsigned char>& coinbase,
                        const std::vector<Target>& branches,
                        uint32_t version, uint32_t time, uint32_t bits,
                        Header& output)
{
    if (coinbase.empty() || coinbase.size() > kMaxCoinbaseBytes ||
        branches.size() > kMaxMerkleDepth)
        return false;

    Target root{};
    CHash256().Write(coinbase.data(), coinbase.size()).Finalize(root.data());
    for (const auto& branch : branches) {
        Target pairHash{};
        CHash256().Write(root.data(), root.size()).Write(branch.data(), branch.size()).Finalize(pairHash.data());
        root = pairHash;
    }
    Header header{};
    auto put32 = [&header](size_t offset, uint32_t word) {
        for (size_t byte = 0; byte < 4; ++byte)
            header[offset + byte] = static_cast<unsigned char>(word >> (8 * byte));
    };
    put32(0, version);
    for (size_t word = 0; word < 32; word += 4)
        std::reverse_copy(stratumPrevious.begin() + word, stratumPrevious.begin() + word + 4,
                          header.begin() + 4 + word);
    std::copy(root.begin(), root.end(), header.begin() + 36);
    put32(68, time);
    put32(72, bits);
    output = header;
    return true;
}

struct BatchResult {
    uint64_t nextNonce;
    uint32_t tested;
    bool found;
    uint32_t nonce;
};

inline uint256 Digest(Header header, uint32_t nonce)
{
    for (size_t byte = 0; byte < 4; ++byte)
        header[76 + byte] = static_cast<unsigned char>(nonce >> (8 * byte));
    return HashQuark(header.begin(), header.end());
}

inline bool Verify(const Header& header, const Target& target, uint32_t nonce)
{
    const auto hash = Digest(header, nonce);
    // Do not compare only the top word: an equal GPU prefix must still
    // pass the complete target comparison on the CPU before submission.
    return !std::lexicographical_compare(target.rbegin(), target.rend(),
                                         std::make_reverse_iterator(hash.end()),
                                         std::make_reverse_iterator(hash.begin()));
}

inline BatchResult ScanCpu(const Header& header, const Target& target, uint64_t first,
                           uint32_t count, const std::atomic<bool>& stop)
{
    if (first >= kNonceEnd) return {kNonceEnd, 0, false, 0};
    const uint64_t limit = std::min(kNonceEnd, first + std::min(count, kMaxCpuBatch));
    BatchResult result{first, 0, false, 0};
    while (result.nextNonce < limit && !stop.load(std::memory_order_relaxed)) {
        const auto nonce = static_cast<uint32_t>(result.nextNonce++);
        ++result.tested;
        if (Verify(header, target, nonce)) {
            result.found = true;
            result.nonce = nonce;
            break;
        }
    }
    return result;
}

// Every lane owns a disjoint range. Resume at the earliest hit + 1 to
// preserve any later hits, including those computed by a different lane.
inline BatchResult ScanCpuParallel(const Header& header, const Target& target, uint64_t first,
                                   uint32_t perLane, unsigned lanes, const std::atomic<bool>& stop)
{
    perLane = std::min(perLane, kMaxCpuBatch);
    if (lanes <= 1 || !perLane || first >= kNonceEnd || stop.load(std::memory_order_relaxed))
        return ScanCpu(header, target, first, perLane, stop);
    std::vector<std::future<BatchResult>> pending;
    for (unsigned lane = 1; lane < lanes; ++lane) {
        const auto start = first + uint64_t{lane} * perLane;
        if (start >= kNonceEnd) break;
        pending.emplace_back(std::async(std::launch::async, [&header, &target, &stop, start, perLane] {
            return ScanCpu(header, target, start, perLane, stop);
        }));
    }
    auto result = ScanCpu(header, target, first, perLane, stop);
    bool contiguous = !result.found && result.tested == perLane;
    for (auto& item : pending) {
        const auto lane = item.get();
        result.tested += lane.tested;
        if (contiguous) {
            result.nextNonce = lane.nextNonce;
            result.found = lane.found;
            result.nonce = lane.nonce;
            contiguous = !lane.found && lane.tested == perLane;
        }
    }
    return result;
}
} // namespace poolwork
#endif
