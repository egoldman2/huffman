#include "archive.hpp"
#include "huffman.hpp"

#include <stdexcept>
#include <string>

namespace huffman {

    namespace {
        void write_integer(std::ostream& output, std::uint64_t value, int byte_count) {
            // write the lowest byte first, then move to the next byte
            for (int i = 0; i < byte_count; ++i) {
                output.put(static_cast<char>(value & 0xFF));
                value >>= 8;
            }
        }

        std::uint64_t read_integer(std::istream& input, int byte_count) {
            std::uint64_t value = 0;

            for (int i = 0; i < byte_count; ++i) {
                char byte = 0;
                if (!input.get(byte)) {
                    throw std::runtime_error("archive header is incomplete or unreadable");
                }

                // put each byte back into its original position
                value |= static_cast<std::uint64_t>(static_cast<unsigned char>(byte)) << (8 * i);
            }

            return value;
        }

        void validate_header(const ArchiveHeader& header) {
            if (header.symbol_count > alphabet_size) {
                throw std::runtime_error("archive has too many symbols");
            }

            if (header.original_size == 0) {
                if (header.symbol_count != 0 || header.bit_count != 0) {
                    throw std::runtime_error("empty archive must have no symbols or bits");
                }
                return;
            }

            if (header.symbol_count == 0 || header.bit_count == 0) {
                throw std::runtime_error("nonempty archive must have symbols and bits");
            }

            if (header.symbol_count > header.original_size || header.bit_count < header.original_size) {
                throw std::runtime_error("archive counts are inconsistent");
            }

            // each byte in a single-symbol file is encoded as one zero bit
            if (header.symbol_count == 1 && header.bit_count != header.original_size) {
                throw std::runtime_error("single-symbol archive must use one bit per byte");
            }
        }
    }

    void write_header(std::ostream& output, const ArchiveHeader& header) {
        validate_header(header);

        // write fields individually so C++ struct padding is never saved
        output.write("HUF1", 4);
        write_integer(output, header.original_size, 8);
        write_integer(output, header.symbol_count, 2);
        write_integer(output, header.bit_count, 8);

        if (!output) {
            throw std::runtime_error("could not write the archive header");
        }
    }

    ArchiveHeader read_header(std::istream& input) {
        char identifier[4]{};
        if (!input.read(identifier, 4)) {
            throw std::runtime_error("archive header is incomplete or unreadable");
        }
        if (std::string(identifier, 4) != "HUF1") {
            throw std::runtime_error("unsupported archive format");
        }

        ArchiveHeader header;
        header.original_size = read_integer(input, 8);
        header.symbol_count = static_cast<std::uint16_t>(read_integer(input, 2));
        header.bit_count = read_integer(input, 8);
        validate_header(header);

        return header;
    }

}
