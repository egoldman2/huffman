#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <vector>
#include <functional>
#include <queue>
#include <utility>

namespace huffman {

    constexpr std::size_t alphabet_size = 256;
    using FrequencyTable = std::array<std::uint64_t, alphabet_size>;

    // struct for representing a node in the tree
    struct Node {
        std::uint64_t frequency; // frequency contained in that node
        int symbol; // the byte value. 0-255 for a leaf, -1 for a parent (sentinel)
        int left = -1; // index left child, default -1, representing no child
        int right = -1; // index left child, default -1, representing no child
    };

    // Counts raw byte values, so this works for text and binary input.
    FrequencyTable count_frequencies(std::istream& input);

    // Creates a tree from a frequency table
    std::vector<Node> create_tree(const FrequencyTable& frequencies);

}  // namespace huffman
