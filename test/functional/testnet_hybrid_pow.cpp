// Test helper: solve an 80-byte header using Agrarian's production Quark hash.
#include "hash.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <thread>
#include <vector>

int main()
{
    std::array<unsigned char, 80> header;
    std::array<unsigned char, 32> target;
    if (std::fread(header.data(), 1, header.size(), stdin) != header.size() ||
        std::fread(target.data(), 1, target.size(), stdin) != target.size())
        return 2;
    const unsigned int jobs = std::max(1u, std::min(3u, std::thread::hardware_concurrency()));
    constexpr uint64_t NOT_FOUND = uint64_t(UINT32_MAX) + 1;
    std::atomic<uint64_t> result{NOT_FOUND};
    std::vector<std::thread> workers;
    for (unsigned int job = 0; job < jobs; ++job) {
        workers.emplace_back([&, job, candidate = header]() mutable {
            for (uint64_t nonce = job; nonce < NOT_FOUND && result.load(std::memory_order_relaxed) == NOT_FOUND; nonce += jobs) {
                for (unsigned int i = 0; i < 4; ++i)
                    candidate[76 + i] = (nonce >> (8 * i)) & 0xff;
                const auto hash = HashQuark(candidate.begin(), candidate.end());
                if (!std::lexicographical_compare(target.begin(), target.end(),
                                                  std::make_reverse_iterator(hash.end()),
                                                  std::make_reverse_iterator(hash.begin()))) {
                    result.store(nonce, std::memory_order_relaxed);
                    break;
                }
            }
        });
    }
    for (auto& worker : workers)
        worker.join();
    if (result != NOT_FOUND) {
        std::printf("%llu\n", static_cast<unsigned long long>(result.load()));
        return 0;
    }
    return 1;
}
