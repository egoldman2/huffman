#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>

namespace huffman {

constexpr std::size_t alphabet_size = 256;
using FrequencyTable = std::array<std::uint64_t, alphabet_size>;

// Counts raw byte values, so this works for text and binary input.
FrequencyTable count_frequencies(std::istream& input);

}  // namespace huffman
