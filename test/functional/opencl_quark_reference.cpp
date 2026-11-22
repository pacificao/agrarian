// Distributed under the MIT software license, see COPYING.
// Emit Agrarian's actual 32-byte Quark digest for each 80-byte input header.
#include "hash.h"

#include <array>
#include <cstdio>

int main()
{
    std::array<unsigned char, 80> header;
    for (;;) {
        const auto bytes = std::fread(header.data(), 1, header.size(), stdin);
        if (bytes == 0) return std::ferror(stdin) ? 1 : 0;
        if (bytes != header.size()) return 2;
        const auto digest = HashQuark(header.begin(), header.end());
        if (std::fwrite(digest.begin(), 1, digest.size(), stdout) != digest.size()) return 3;
    }
}
