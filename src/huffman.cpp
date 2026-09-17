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

std::vector<Node> create_tree(const FrequencyTable& frequencies) {
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
    while (queue.size() > 1) {

        // remove the two nodes with the lowest frequencies
        // store the indicies
        int left_index = queue.top().second;
        queue.pop();

        int right_index = queue.top().second;
        queue.pop();

        // calculate combined frequency
        // this is used for the parent node of the popped nodes
        const std::uint64_t combined_frequency = nodes[left_index].frequency + nodes[right_index].frequency;

        // get the current node size
        // this is important as we always add the parent node to the end
        // the results in the last parent node always representing the top of the whole tree
        const int parent_index = static_cast<int>(nodes.size());

        // create a parent:
        nodes.push_back(Node{
            combined_frequency, // frequency = left frequency + right frequency
            -1, // symbol = -1, as the parent node does not represent a symbol
            left_index, // left = first node position
            right_index // right = second node position
        });

        // add the parent to the queue 
        queue.push({combined_frequency, parent_index});
    }

    return nodes;
}



// Next experiment:
// 1. Make one leaf for every non-zero frequency.
// 2. Put those leaves in a minimum-priority queue.
// 3. Remove and merge the two lightest trees until one remains.

}  // namespace huffman
