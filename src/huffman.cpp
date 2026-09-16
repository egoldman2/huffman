#include "huffman.hpp"

namespace huffman {

FrequencyTable count_frequencies(std::istream& input) {
    FrequencyTable frequencies{};
    char byte = 0;

    while (input.get(byte)) {
        const auto symbol = static_cast<unsigned char>(byte);
        ++frequencies[symbol];
    }

    return frequencies;
}

// Next experiment:
// 1. Make one leaf for every non-zero frequency.
// 2. Put those leaves in a minimum-priority queue.
// 3. Remove and merge the two lightest trees until one remains.

}  // namespace huffman
