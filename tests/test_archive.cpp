#include "archive.hpp"
#include "huffman.hpp"

#include <gtest/gtest.h>

#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

TEST(ArchiveHeader, WritesKnownBytesInLittleEndianOrder) {
    std::ostringstream output;
    huffman::write_header(output, {6, 3, 9});

    const std::string expected = {
        'H', 'U', 'F', '1',
        6, 0, 0, 0, 0, 0, 0, 0,
        3, 0,
        9, 0, 0, 0, 0, 0, 0, 0
    };
    EXPECT_EQ(output.str(), expected);
}

TEST(ArchiveHeader, ReadsKnownBytesWithoutConsumingTheTree) {
    const std::string bytes = {
        'H', 'U', 'F', '1',
        6, 0, 0, 0, 0, 0, 0, 0,
        3, 0,
        9, 0, 0, 0, 0, 0, 0, 0,
        1
    };
    std::istringstream input(bytes);
    const auto header = huffman::read_header(input);

    EXPECT_EQ(header.original_size, std::uint64_t{6});
    EXPECT_EQ(header.symbol_count, std::uint16_t{3});
    EXPECT_EQ(header.bit_count, std::uint64_t{9});
    EXPECT_EQ(input.get(), 1);
}

TEST(ArchiveHeader, PreservesEmptySingleSymbolAndLargeCounts) {
    const huffman::ArchiveHeader examples[] = {
        {0, 0, 0}, {4, 1, 4}, {256, 256, 2048},
        {std::numeric_limits<std::uint64_t>::max(), 1,
         std::numeric_limits<std::uint64_t>::max()}
    };

    for (const auto& expected : examples) {
        std::ostringstream output;
        huffman::write_header(output, expected);
        std::istringstream input(output.str());
        const auto actual = huffman::read_header(input);

        EXPECT_EQ(actual.original_size, expected.original_size);
        EXPECT_EQ(actual.symbol_count, expected.symbol_count);
        EXPECT_EQ(actual.bit_count, expected.bit_count);
    }
}

TEST(ArchiveHeader, RejectsEveryTruncatedHeader) {
    std::ostringstream output;
    huffman::write_header(output, {6, 3, 9});
    const auto bytes = output.str();

    for (std::size_t length = 0; length < bytes.size(); ++length) {
        SCOPED_TRACE(length);
        std::istringstream input(bytes.substr(0, length));
        EXPECT_THROW(huffman::read_header(input), std::runtime_error);
    }
}

TEST(ArchiveHeader, RejectsUnsupportedIdentifiersAndInvalidCounts) {
    std::ostringstream output;
    huffman::write_header(output, {6, 3, 9});
    auto bytes = output.str();
    bytes[3] = '2';
    std::istringstream unsupported(bytes);
    EXPECT_THROW(huffman::read_header(unsupported), std::runtime_error);

    // change the symbol count to 257 without using the validating writer
    bytes[3] = '1';
    bytes[12] = 1;
    bytes[13] = 1;
    std::istringstream invalid(bytes);
    EXPECT_THROW(huffman::read_header(invalid), std::runtime_error);

    const huffman::ArchiveHeader bad_headers[] = {
        {0, 1, 0}, {0, 0, 1}, {6, 0, 9}, {6, 3, 0},
        {1, 2, 2}, {6, 3, 5}, {4, 1, 5}, {300, 257, 300}
    };
    for (const auto& header : bad_headers) {
        std::ostringstream destination;
        EXPECT_THROW(huffman::write_header(destination, header), std::runtime_error);
        EXPECT_TRUE(destination.str().empty());
    }
}

TEST(ArchiveHeader, ReportsWriteFailures) {
    std::ostringstream output;
    output.setstate(std::ios::badbit);
    EXPECT_THROW(huffman::write_header(output, {6, 3, 9}), std::runtime_error);
}

TEST(Compression, WritesTheDocumentedBananaArchive) {
    std::istringstream input("BANANA");
    std::ostringstream output;
    const auto header = huffman::compress(input, output);

    const std::string expected = {
        'H', 'U', 'F', '1',
        6, 0, 0, 0, 0, 0, 0, 0,
        3, 0,
        9, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 'A', 0, 1, 'B', 1, 'N',
        static_cast<char>(0x9B), 0
    };
    EXPECT_EQ(output.str(), expected);
    EXPECT_EQ(header.original_size, std::uint64_t{6});
    EXPECT_EQ(header.symbol_count, std::uint16_t{3});
    EXPECT_EQ(header.bit_count, std::uint64_t{9});
}

TEST(Compression, WritesEmptyAndSingleSymbolArchives) {
    std::istringstream empty_input("");
    std::ostringstream empty_output;
    huffman::compress(empty_input, empty_output);
    EXPECT_EQ(empty_output.str(), "HUF1" + std::string(18, '\0'));

    std::istringstream single_input("XXXX");
    std::ostringstream single_output;
    huffman::compress(single_input, single_output);
    ASSERT_EQ(single_output.str().size(), std::size_t{25});
    EXPECT_EQ(single_output.str().substr(22), (std::string{1, 'X', 0}));
}

TEST(Compression, PreservesBinaryInputAndTheFullAlphabet) {
    std::string original;
    for (int symbol = 0; symbol < 256; ++symbol) {
        original += static_cast<char>(symbol);
    }
    // also exercise repeated symbols and a payload larger than a tiny example
    original += std::string(10000, 'A');
    std::istringstream input(original);
    std::ostringstream output;
    huffman::compress(input, output);

    std::istringstream saved(output.str());
    const auto header = huffman::read_header(saved);
    const auto tree = huffman::read_tree(saved, header.symbol_count);
    std::vector<unsigned char> packed;
    char byte = 0;
    while (saved.get(byte)) {
        packed.push_back(static_cast<unsigned char>(byte));
    }

    EXPECT_EQ(header.symbol_count, std::uint16_t{256});
    EXPECT_EQ(header.original_size, original.size());
    EXPECT_EQ(packed.size(), header.bit_count / 8 + (header.bit_count % 8 != 0));
    const auto bits = huffman::unpack_bits(packed, header.bit_count);
    EXPECT_EQ(huffman::decode(bits, tree), original);
}

TEST(Compression, StartsAtTheCurrentInputPosition) {
    std::istringstream input("skipBANANA");
    input.seekg(4);
    std::ostringstream output;
    const auto header = huffman::compress(input, output);

    EXPECT_EQ(header.original_size, std::uint64_t{6});
    EXPECT_EQ(header.bit_count, std::uint64_t{9});
    EXPECT_EQ(output.str().substr(30), (std::string{static_cast<char>(0x9B), 0}));
}

TEST(Compression, ReportsUnreadableInputAndWriteFailures) {
    std::istringstream bad_input("BANANA");
    bad_input.setstate(std::ios::badbit);
    std::ostringstream output;
    EXPECT_THROW(huffman::compress(bad_input, output), std::runtime_error);
    EXPECT_TRUE(output.str().empty());

    std::istringstream input("BANANA");
    output.setstate(std::ios::badbit);
    EXPECT_THROW(huffman::compress(input, output), std::runtime_error);
}

TEST(Decompression, ReadsTheDocumentedBananaArchive) {
    const std::string bytes = {
        'H', 'U', 'F', '1',
        6, 0, 0, 0, 0, 0, 0, 0,
        3, 0,
        9, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 'A', 0, 1, 'B', 1, 'N',
        static_cast<char>(0x9B), 0
    };
    std::istringstream input(bytes);
    std::ostringstream output;
    const auto header = huffman::decompress(input, output);

    EXPECT_EQ(output.str(), "BANANA");
    EXPECT_EQ(header.original_size, std::uint64_t{6});
}

TEST(Decompression, RestoresEmptySingleSymbolAndBinaryArchives) {
    std::string alphabet;
    for (int symbol = 0; symbol < 256; ++symbol) {
        alphabet += static_cast<char>(symbol);
    }
    const std::vector<std::string> examples = {
        "", "A", std::string(7, 'X'), std::string(8, 'X'),
        std::string(9, 'X'), std::string(17, 'X'), "BANANA",
        alphabet, alphabet + std::string(10000, 'A')
    };

    for (const auto& original : examples) {
        SCOPED_TRACE(original.size());
        std::istringstream input(original);
        std::ostringstream archive;
        huffman::compress(input, archive);
        std::istringstream saved(archive.str());
        std::ostringstream restored;
        huffman::decompress(saved, restored);

        EXPECT_EQ(restored.str(), original);
    }
}

TEST(Decompression, RestoresSeededRandomInputs) {
    std::mt19937 generator(42);
    for (int length = 0; length < 100; ++length) {
        SCOPED_TRACE(length);
        std::string original;
        for (int i = 0; i < length; ++i) {
            original += static_cast<char>(generator() % 256);
        }
        std::istringstream input(original);
        std::ostringstream archive;
        huffman::compress(input, archive);
        std::istringstream saved(archive.str());
        std::ostringstream restored;
        huffman::decompress(saved, restored);

        EXPECT_EQ(restored.str(), original);
    }
}

TEST(Decompression, RejectsEveryTruncatedArchive) {
    std::istringstream input("BANANA");
    std::ostringstream archive;
    huffman::compress(input, archive);
    const auto bytes = archive.str();

    for (std::size_t length = 0; length < bytes.size(); ++length) {
        SCOPED_TRACE(length);
        std::istringstream saved(bytes.substr(0, length));
        std::ostringstream output;
        EXPECT_THROW(huffman::decompress(saved, output), std::runtime_error);
    }
}

TEST(Decompression, RejectsPaddingTrailingDataAndWrongSizes) {
    std::istringstream input("BANANA");
    std::ostringstream archive;
    huffman::compress(input, archive);
    const auto valid = archive.str();

    auto bad_padding = valid;
    bad_padding.back() = 1;
    auto too_few_bytes = valid;
    too_few_bytes[4] = 7;
    auto too_many_bytes = valid;
    too_many_bytes[4] = 5;
    auto incomplete_symbol = valid;
    incomplete_symbol[14] = 8;

    const std::vector<std::string> invalid_archives = {
        bad_padding, valid + "extra", too_few_bytes, too_many_bytes, incomplete_symbol,
        "HUF1" + std::string(18, '\0') + "extra"
    };
    for (std::size_t i = 0; i < invalid_archives.size(); ++i) {
        SCOPED_TRACE(i);
        std::istringstream saved(invalid_archives[i]);
        std::ostringstream output;
        EXPECT_THROW(huffman::decompress(saved, output), std::runtime_error);
    }
}

TEST(Decompression, RejectsInvalidSingleSymbolBitsAndMalformedTrees) {
    std::istringstream input("XXXX");
    std::ostringstream archive;
    huffman::compress(input, archive);
    auto bytes = archive.str();
    bytes.back() = static_cast<char>(0x80);
    std::istringstream invalid_bits(bytes);
    std::ostringstream output;
    EXPECT_THROW(huffman::decompress(invalid_bits, output), std::runtime_error);

    bytes = archive.str();
    bytes[22] = 2;
    std::istringstream invalid_tree(bytes);
    EXPECT_THROW(huffman::decompress(invalid_tree, output), std::runtime_error);
}

TEST(Decompression, ReportsWriteFailures) {
    std::istringstream input("BANANA");
    std::ostringstream archive;
    huffman::compress(input, archive);
    std::istringstream saved(archive.str());
    std::ostringstream output;
    output.setstate(std::ios::badbit);
    EXPECT_THROW(huffman::decompress(saved, output), std::runtime_error);
}
