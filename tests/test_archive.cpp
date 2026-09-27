#include "archive.hpp"

#include <gtest/gtest.h>

#include <limits>
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
