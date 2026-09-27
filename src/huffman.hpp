#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <vector>
#include <functional>
#include <queue>
#include <utility>
#include <string>
#include <bitset>

namespace huffman {

    constexpr std::size_t alphabet_size = 256;
    using FrequencyTable = std::array<std::uint64_t, alphabet_size>;

    // string stores the binary value, index is the symbol value
    using CodeTable = std::array<std::string, alphabet_size>;

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

    // Creates binary code values for all the nodes in the tree
    CodeTable create_codes(const std::vector<Node>& tree);

    // Save a tree from create_tree or read_tree using the HUF1 markers
    void write_tree(std::ostream& output, const std::vector<Node>& tree);

    // Load a tree using the symbol count from the archive header
    std::vector<Node> read_tree(std::istream& input, std::size_t symbol_count);

    // Encode an input string into a binary, huffman string
    std::string encode(
        const std::string& input,
        const CodeTable& codes
    );

    // Decode a binary, huffman string, back into data
    // Uses a tree from create_tree; throws std::invalid_argument for invalid bits
    std::string decode(
        const std::string& bits,
        const std::vector<Node>& tree
    );

    // Pack bits into actual bytes so that it actually stores raw bytes
    std::vector<unsigned char> pack_bits(
        const std::string& bits
    );

    // Unpack bytes into bits, removing padding using the original bit count
    std::string unpack_bits(
        const std::vector<unsigned char>& packed,
        std::size_t bit_count
    );

}  // namespace huffman
