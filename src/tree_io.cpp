#include "huffman.hpp"

#include <stdexcept>

namespace huffman {

    namespace {
        void write_node(
            std::ostream& output,
            const std::vector<Node>& tree,
            int node_index
        ) {
            const Node& node = tree[node_index];

            if (node.symbol != -1) {
                // a leaf marker is followed by the actual symbol byte
                output.put(static_cast<char>(1));
                output.put(static_cast<char>(node.symbol));
                return;
            }

            // save the parent marker before its left and right children
            output.put(static_cast<char>(0));
            write_node(output, tree, node.left);
            write_node(output, tree, node.right);
        }

        unsigned char read_tree_byte(std::istream& input) {
            char byte = 0;
            if (!input.get(byte)) {
                throw std::runtime_error("saved tree is incomplete or unreadable");
            }
            return static_cast<unsigned char>(byte);
        }

        int read_node(
            std::istream& input,
            std::vector<Node>& tree,
            std::array<bool, alphabet_size>& seen_symbols,
            std::size_t symbol_count,
            std::size_t& nodes_read,
            std::size_t& leaves_read,
            std::size_t depth
        ) {
            // count markers before reading children, so unfinished parents also count
            if (depth > 255 || nodes_read >= 2 * symbol_count - 1) {
                throw std::runtime_error("saved tree exceeds its size or depth limit");
            }
            ++nodes_read;

            const unsigned char marker = read_tree_byte(input);
            if (marker == 1) {
                const unsigned char symbol = read_tree_byte(input);
                if (seen_symbols[symbol]) {
                    throw std::runtime_error("saved tree contains a duplicate symbol");
                }
                seen_symbols[symbol] = true;
                ++leaves_read;

                tree.push_back(Node{0, symbol, -1, -1});
                return static_cast<int>(tree.size()) - 1;
            }

            if (marker != 0) {
                throw std::runtime_error("saved tree contains an invalid marker");
            }

            const int left_index = read_node(
                input, tree, seen_symbols, symbol_count, nodes_read, leaves_read, depth + 1
            );
            const int right_index = read_node(
                input, tree, seen_symbols, symbol_count, nodes_read, leaves_read, depth + 1
            );

            // append the parent after its children, keeping the root at the end
            // frequencies are not needed when decoding a saved tree
            tree.push_back(Node{0, -1, left_index, right_index});
            return static_cast<int>(tree.size()) - 1;
        }
    }

    void write_tree(std::ostream& output, const std::vector<Node>& tree) {
        if (!tree.empty()) {
            const int root_index = static_cast<int>(tree.size()) - 1;
            write_node(output, tree, root_index);
        }

        if (!output) {
            throw std::runtime_error("could not write the tree");
        }
    }

    std::vector<Node> read_tree(std::istream& input, std::size_t symbol_count) {
        if (symbol_count > alphabet_size) {
            throw std::runtime_error("saved tree has too many symbols");
        }

        std::vector<Node> tree;
        if (symbol_count == 0) {
            return tree;
        }

        std::array<bool, alphabet_size> seen_symbols{};
        std::size_t nodes_read = 0;
        std::size_t leaves_read = 0;
        read_node(input, tree, seen_symbols, symbol_count, nodes_read, leaves_read, 0);

        if (leaves_read != symbol_count) {
            throw std::runtime_error("saved tree does not match the symbol count");
        }

        return tree;
    }

}
