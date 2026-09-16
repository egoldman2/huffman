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

std::vector<Node> create_tree(const FrequencyTable frequencies) {
    // frequency, node index
    using QueueItem = std::pair<std::uint64_t, int>;

    // create the list of nodes
    // this stores TOTAL frequency of the node, symbol, and left and right children
    // left and right are -1 if no children exist (leaf node)
    std::vector<Node> nodes;

    // create a minimum priority queue, to store the list of frequencies from least to most occuring
    // this is then used to construct the tree
    std::priority_queue<
        QueueItem,
        std::vector<QueueItem>,
        std::greater<QueueItem>
    > queue;

    // go through all possible bytes
    for (std::size_t symbol = 0; symbol < frequencies.size(); ++symbol) {
        // skip if the symbol doesnt occur
        if (frequencies[symbol] == 0) {
            continue;
        }

        // record where the new node will be stored in the vector
        const int node_index = static_cast<int>(nodes.size());

        // create a leaf node. children are -1 as leaves do not have children
        // there are no children yet as the tree hasnt been constructed
        nodes.push_back(Node{
            frequencies[symbol],
            static_cast<int>(symbol),
            -1,
            -1
        });

        // add leaf's frequency and location to min-priority queue
        queue.push({frequencies[symbol], node_index});
    }

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
