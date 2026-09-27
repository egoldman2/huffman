#include "huffman.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>

TEST(FrequencyCounting, CountsRawByteValues) {
    const std::string bytes = {'A', '\0', 'A', static_cast<char>(0xFF)};
    std::istringstream input(bytes);
    const auto frequencies = huffman::count_frequencies(input);

    EXPECT_EQ(frequencies[static_cast<unsigned char>('A')], std::uint64_t{2});
    EXPECT_EQ(frequencies[0x00], std::uint64_t{1});
    EXPECT_EQ(frequencies[0xFF], std::uint64_t{1});
    EXPECT_EQ(frequencies[static_cast<unsigned char>('B')], std::uint64_t{0});
}

TEST(TreeCreation, HandlesEmptyInput) {
    const huffman::FrequencyTable frequencies{};

    EXPECT_TRUE(huffman::create_tree(frequencies).empty());
}

TEST(TreeCreation, HandlesOneSymbol) {
    huffman::FrequencyTable frequencies{};
    frequencies[static_cast<unsigned char>('X')] = 4;
    const auto tree = huffman::create_tree(frequencies);

    ASSERT_EQ(tree.size(), std::size_t{1});
    EXPECT_EQ(tree.back().frequency, std::uint64_t{4});
    EXPECT_EQ(tree.back().symbol, 'X');
    EXPECT_EQ(tree.back().left, -1);
    EXPECT_EQ(tree.back().right, -1);

    const auto codes = huffman::create_codes(tree);
    EXPECT_EQ(codes[static_cast<unsigned char>('X')], "0");
}

TEST(TreeCreation, BuildsBananaTree) {
    huffman::FrequencyTable frequencies{};
    frequencies[static_cast<unsigned char>('A')] = 3;
    frequencies[static_cast<unsigned char>('N')] = 2;
    frequencies[static_cast<unsigned char>('B')] = 1;
    const auto tree = huffman::create_tree(frequencies);

    ASSERT_EQ(tree.size(), std::size_t{5});

    const auto& root = tree.back();
    EXPECT_EQ(root.frequency, std::uint64_t{6});
    EXPECT_EQ(root.symbol, -1);
    ASSERT_GE(root.left, 0);
    ASSERT_GE(root.right, 0);
    ASSERT_LT(static_cast<std::size_t>(root.left), tree.size());
    ASSERT_LT(static_cast<std::size_t>(root.right), tree.size());
    EXPECT_EQ(tree[root.left].frequency + tree[root.right].frequency,
              root.frequency);
}

TEST(CodeGeneration, CreatesBananaCodes) {
    huffman::FrequencyTable frequencies{};
    frequencies[static_cast<unsigned char>('A')] = 3;
    frequencies[static_cast<unsigned char>('N')] = 2;
    frequencies[static_cast<unsigned char>('B')] = 1;

    const auto tree = huffman::create_tree(frequencies);
    const auto codes = huffman::create_codes(tree);

    EXPECT_EQ(codes[static_cast<unsigned char>('A')], "0");
    EXPECT_EQ(codes[static_cast<unsigned char>('B')], "10");
    EXPECT_EQ(codes[static_cast<unsigned char>('N')], "11");
}

TEST(BitPacking, HandlesEmptyInput) {
    EXPECT_TRUE(huffman::pack_bits("").empty());
}

TEST(Decoding, RestoresBanana) {
    std::istringstream input("BANANA");
    const auto tree = huffman::create_tree(huffman::count_frequencies(input));
    const auto codes = huffman::create_codes(tree);

    EXPECT_EQ(huffman::decode("100110110", tree), "BANANA");
    EXPECT_EQ(huffman::decode(huffman::encode("BANANA", codes), tree), "BANANA");
    EXPECT_TRUE(huffman::decode("", tree).empty());
}

TEST(Decoding, HandlesEmptyTree) {
    EXPECT_TRUE(huffman::decode("", {}).empty());
    EXPECT_THROW(huffman::decode("0", {}), std::invalid_argument);
}

TEST(Decoding, HandlesOneSymbol) {
    std::istringstream input("XXXX");
    const auto tree = huffman::create_tree(huffman::count_frequencies(input));

    EXPECT_EQ(huffman::decode("0000", tree), "XXXX");
    EXPECT_TRUE(huffman::decode("", tree).empty());
    EXPECT_THROW(huffman::decode("1", tree), std::invalid_argument);
}

TEST(Decoding, RestoresRawBytes) {
    const std::string bytes = {'A', '\0', 'A', static_cast<char>(0xFF)};
    std::istringstream input(bytes);
    const auto tree = huffman::create_tree(huffman::count_frequencies(input));
    const auto codes = huffman::create_codes(tree);

    EXPECT_EQ(huffman::decode(huffman::encode(bytes, codes), tree), bytes);
}

TEST(Decoding, RejectsInvalidBitsAndIncompleteSymbols) {
    std::istringstream input("BANANA");
    const auto tree = huffman::create_tree(huffman::count_frequencies(input));

    EXPECT_THROW(huffman::decode("02", tree), std::invalid_argument);
    EXPECT_THROW(huffman::decode("1", tree), std::invalid_argument);
}

TEST(BitPacking, PadsPartialByteOnTheRight) {
    const auto partial = huffman::pack_bits("101");
    ASSERT_EQ(partial.size(), std::size_t{1});
    EXPECT_EQ(partial[0], static_cast<unsigned char>(0b10100000));
}

TEST(BitPacking, PacksAcrossTwoBytes) {
    const auto example = huffman::pack_bits("111110000010011");
    ASSERT_EQ(example.size(), std::size_t{2});
    EXPECT_EQ(example[0], static_cast<unsigned char>(0xF8));
    EXPECT_EQ(example[1], static_cast<unsigned char>(0x26));
}

TEST(BitUnpacking, HandlesEmptyInput) {
    EXPECT_TRUE(huffman::unpack_bits({}, 0).empty());
}

TEST(BitUnpacking, RemovesPaddingFromPartialByte) {
    EXPECT_EQ(huffman::unpack_bits({0xA0}, 3), "101");
}

TEST(BitUnpacking, RestoresFullBytesInOrder) {
    EXPECT_EQ(huffman::unpack_bits({0xF8, 0x26}, 16), "1111100000100110");
    EXPECT_EQ(huffman::unpack_bits({0xF8, 0x26}, 15), "111110000010011");
}

TEST(BitUnpacking, RejectsBitCountsBeyondAvailableData) {
    EXPECT_THROW(huffman::unpack_bits({}, 1), std::invalid_argument);
    EXPECT_THROW(huffman::unpack_bits({0xA0}, 9), std::invalid_argument);
}

TEST(RoundTrip, RestoresInputAfterPackingAndUnpacking) {
    const std::vector<std::string> examples = {
        "", "XXXX", "BANANA",
        std::string{'A', '\0', 'A', static_cast<char>(0xFF)}
    };

    for (const std::string& original : examples) {
        SCOPED_TRACE(::testing::PrintToString(original));
        std::istringstream input(original);
        const auto tree = huffman::create_tree(huffman::count_frequencies(input));
        const auto codes = huffman::create_codes(tree);
        const auto bits = huffman::encode(original, codes);
        const auto packed = huffman::pack_bits(bits);
        const auto unpacked = huffman::unpack_bits(packed, bits.size());

        EXPECT_EQ(unpacked, bits);
        EXPECT_EQ(huffman::decode(unpacked, tree), original);
    }
}

TEST(TreeStorage, WritesTheDocumentedBananaBytes) {
    std::istringstream input("BANANA");
    const auto tree = huffman::create_tree(huffman::count_frequencies(input));
    std::ostringstream saved;
    huffman::write_tree(saved, tree);

    const std::string expected = {0, 1, 'A', 0, 1, 'B', 1, 'N'};
    EXPECT_EQ(saved.str(), expected);
}

TEST(TreeStorage, LoadsKnownBytesAndLeavesThePayloadUnread) {
    const std::string bytes = {0, 1, 'A', 0, 1, 'B', 1, 'N', 'P'};
    std::istringstream saved(bytes);
    const auto tree = huffman::read_tree(saved, 3);

    ASSERT_EQ(tree.size(), std::size_t{5});
    EXPECT_EQ(tree.back().symbol, -1);
    EXPECT_EQ(huffman::decode("100110110", tree), "BANANA");
    EXPECT_EQ(saved.get(), 'P');
}

TEST(TreeStorage, HandlesEmptyTreesWithoutReadingData) {
    std::ostringstream output;
    huffman::write_tree(output, {});
    EXPECT_TRUE(output.str().empty());

    std::istringstream input("P");
    EXPECT_TRUE(huffman::read_tree(input, 0).empty());
    EXPECT_EQ(input.get(), 'P');
}

TEST(TreeStorage, PreservesCodesAndBinarySymbols) {
    const std::vector<std::string> examples = {
        "XXXX", "BANANA", std::string{0, 1, static_cast<char>(0xFF)}
    };

    for (const std::string& original : examples) {
        SCOPED_TRACE(::testing::PrintToString(original));
        std::istringstream input(original);
        const auto frequencies = huffman::count_frequencies(input);
        const auto tree = huffman::create_tree(frequencies);
        std::size_t symbol_count = 0;
        for (auto frequency : frequencies) {
            if (frequency != 0) {
                ++symbol_count;
            }
        }

        std::ostringstream output;
        huffman::write_tree(output, tree);
        std::istringstream saved(output.str());
        const auto restored_tree = huffman::read_tree(saved, symbol_count);
        const auto codes = huffman::create_codes(tree);

        EXPECT_EQ(huffman::create_codes(restored_tree), codes);
        EXPECT_EQ(huffman::decode(huffman::encode(original, codes), restored_tree), original);
    }
}

TEST(TreeStorage, RejectsMalformedTrees) {
    std::istringstream invalid_marker(std::string{2});
    EXPECT_THROW(huffman::read_tree(invalid_marker, 1), std::runtime_error);

    std::istringstream truncated_leaf(std::string{1});
    EXPECT_THROW(huffman::read_tree(truncated_leaf, 1), std::runtime_error);

    std::istringstream missing_child(std::string{0, 1, 'A'});
    EXPECT_THROW(huffman::read_tree(missing_child, 2), std::runtime_error);

    std::istringstream duplicate(std::string{0, 1, 'A', 1, 'A'});
    EXPECT_THROW(huffman::read_tree(duplicate, 2), std::runtime_error);

    std::istringstream wrong_count(std::string{1, 'A'});
    EXPECT_THROW(huffman::read_tree(wrong_count, 2), std::runtime_error);

    std::istringstream too_many_nodes(std::string{0, 0, 0});
    EXPECT_THROW(huffman::read_tree(too_many_nodes, 2), std::runtime_error);

    std::istringstream too_many_symbols;
    EXPECT_THROW(huffman::read_tree(too_many_symbols, 257), std::runtime_error);
}

TEST(TreeStorage, ReportsWriteFailures) {
    std::istringstream input("X");
    const auto tree = huffman::create_tree(huffman::count_frequencies(input));
    std::ostringstream output;
    output.setstate(std::ios::badbit);

    EXPECT_THROW(huffman::write_tree(output, tree), std::runtime_error);
}
