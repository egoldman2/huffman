#pragma once

#include <cstdint>
#include <istream>
#include <ostream>

namespace huffman {

    // metadata needed to read the tree and decode the payload
    struct ArchiveHeader {
        std::uint64_t original_size = 0;
        std::uint16_t symbol_count = 0;
        std::uint64_t bit_count = 0;
    };

    // Write the HUF1 identifier and fields in little-endian order
    void write_header(std::ostream& output, const ArchiveHeader& header);

    // Read and validate the fixed 22-byte header
    ArchiveHeader read_header(std::istream& input);

}
