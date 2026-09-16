#include "huffman.hpp"

#include <cassert>
#include <sstream>
#include <string>

int main() {
    const std::string bytes = {'A', '\0', 'A', static_cast<char>(0xFF)};
    std::istringstream input(bytes);

    const auto frequencies = huffman::count_frequencies(input);

    assert(frequencies[static_cast<unsigned char>('A')] == 2);
    assert(frequencies[0x00] == 1);
    assert(frequencies[0xFF] == 1);
    assert(frequencies[static_cast<unsigned char>('B')] == 0);
}
