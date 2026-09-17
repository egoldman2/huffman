#include "huffman.hpp"

namespace huffman {

    // another namespace for private helper functions 
    namespace {
        // function that we can call recursively to set a code starting at a specificed node index
        // we walk the entire tree using this function
        // 0 is ammended to left children, 1 to right (arbitrarily, it could be swapped and still work)
        void fill_codes(
            const std::vector<Node>& tree,
            int node_index,
            const std::string& current_code,
            CodeTable& codes
        ) {
            const Node& node = tree[node_index];
            
            // if the symbol is not -1, then the node is a leaf
            if (node.symbol != -1) {
                //set the code for the symbol
                // if there arent any codes yet, then we have just started, so start the code at 0
                codes[node.symbol] = current_code.empty() ? "0" : current_code;
                return;
            }

            // if the code gets here, it means that its not a leaf, so process the left and right children
            // respectively, either add a "0" or "1" to the current code
            fill_codes(tree, node.left, current_code + "0", codes);
            fill_codes(tree, node.right, current_code + "1", codes);
        }
    }

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

    CodeTable create_codes(const std::vector<Node>& tree) {

        CodeTable codes{};

        if (tree.empty()) {
            return codes;
        }

        // get the root of the tree. the root of the tree is always the last element stored in the tree
        // we use -1 as indices start at 0 while size starts at 1
        const int root_index = static_cast<int>(tree.size()) - 1;

        // start filling codes starting at the root
        // the function calls itself recursively, so once it completes, it means all codes have been assigned
        fill_codes(tree, root_index, "", codes);

        return codes;
    }

    std::string encode(
        const std::string& input,
        const CodeTable& codes
    ) {
        std::string bits;

        for (char byte : input) {
            const auto symbol = static_cast<unsigned char>(byte);

            // Append the Huffman code for this byte.
            bits += codes[symbol];
        }

        return bits;
    }

    std::vector<unsigned char> pack_bits(const std::string& bits) {

        // variable to store packed bytes
        std::vector<unsigned char> packed;

        // we use += 8 to skip to first bit of the next byte on each iteration
        for (std::size_t i = 0; i < bits.size(); i += 8) {
            // get a "chunk" by extracting from the current index, up to 8 characters
            std::string chunk = bits.substr(i, 8);

            // this is a little cooked
            // 1. create a fake byte from between 1-8 bits (bitset appends missing 0s if < 8 bits)
            // 2. convert the value of the fake byte back to an ordinary number
            // 3. convert the ordinary number into an actual byte
            packed.push_back(
                static_cast<unsigned char>(
                    std::bitset<8>(chunk).to_ulong()
                )
            );
        }

        return packed;
    }

}