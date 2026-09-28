#include "file_ops.hpp"

#include <exception>
#include <iostream>
#include <string>

// small command-line entry point for benchmarking the same helpers as the menu
int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: driver compress|decompress input output\n";
        return 1;
    }

    try {
        const std::string operation = argv[1];
        if (operation == "compress") {
            huffman::compress_file(argv[2], argv[3]);
        } else if (operation == "decompress") {
            huffman::decompress_file(argv[2], argv[3]);
        } else {
            std::cerr << "Unknown operation\n";
            return 1;
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    return 0;
}
