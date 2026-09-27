#include "archive.hpp"
#include "huffman.hpp"

#include <stdexcept>
#include <string>
#include <limits>

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

    ArchiveHeader compress(std::istream& input, std::ostream& output) {
        // remember where to start again after counting the frequencies
        const auto start_position = input.tellg();
        if (start_position == std::streampos(-1)) {
            throw std::runtime_error("compression requires a readable, seekable input");
        }

        const auto frequencies = count_frequencies(input);
        if (input.bad() || !input.eof()) {
            throw std::runtime_error("could not read the input");
        }

        ArchiveHeader header;
        const auto maximum = std::numeric_limits<std::uint64_t>::max();
        for (auto frequency : frequencies) {
            if (frequency != 0) {
                if (frequency > maximum - header.original_size) {
                    throw std::overflow_error("original size is too large");
                }
                header.original_size += frequency;
                ++header.symbol_count;
            }
        }

        const auto tree = create_tree(frequencies);
        const auto codes = create_codes(tree);

        // work out the payload size before writing the header
        for (std::size_t symbol = 0; symbol < frequencies.size(); ++symbol) {
            const auto code_length = codes[symbol].size();
            if (code_length != 0) {
                if (frequencies[symbol] > (maximum - header.bit_count) / code_length) {
                    throw std::overflow_error("encoded bit count is too large");
                }
                header.bit_count += frequencies[symbol] * code_length;
            }
        }

        // clear EOF from the first pass and return to the original position
        input.clear();
        input.seekg(start_position);
        if (!input) {
            throw std::runtime_error("could not rewind the input");
        }

        write_header(output, header);
        write_tree(output, tree);

        // keep only a small group of pending bits, rather than the whole payload
        std::string pending_bits;
        auto remaining = frequencies;
        char byte = 0;
        while (input.get(byte)) {
            const auto symbol = static_cast<unsigned char>(byte);
            if (remaining[symbol] == 0) {
                throw std::runtime_error("input frequencies changed during compression");
            }
            --remaining[symbol];
            pending_bits += codes[symbol];

            while (pending_bits.size() >= 8) {
                const auto packed = pack_bits(pending_bits.substr(0, 8));
                output.put(static_cast<char>(packed[0]));
                pending_bits.erase(0, 8);
                if (!output) {
                    throw std::runtime_error("could not write the compressed payload");
                }
            }
        }

        if (input.bad() || !input.eof()) {
            throw std::runtime_error("could not read the input");
        }
        for (auto frequency : remaining) {
            if (frequency != 0) {
                throw std::runtime_error("input frequencies changed during compression");
            }
        }

        // pack_bits adds trailing zero padding to the final partial byte
        if (!pending_bits.empty()) {
            const auto packed = pack_bits(pending_bits);
            output.put(static_cast<char>(packed[0]));
        }
        if (!output) {
            throw std::runtime_error("could not write the compressed payload");
        }

        return header;
    }

}
