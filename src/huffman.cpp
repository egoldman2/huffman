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

std::vector<Node> create_tree(FrequencyTable frequencies) {
    // for each possible byte:
    //     if its frequency is greater than zero:
    //         add a leaf node
    //         put that node into the queue

    // while the queue contains more than one node:
    //     remove the two nodes with the lowest frequencies

    //     create a parent:
    //         frequency = left frequency + right frequency
    //         symbol = -1
    //         left = first node position
    //         right = second node position

    //     add the parent to the queue

    // the final queue entry is the root
}



// Next experiment:
// 1. Make one leaf for every non-zero frequency.
// 2. Put those leaves in a minimum-priority queue.
// 3. Remove and merge the two lightest trees until one remains.

}  // namespace huffman
