#include "huffman.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <sstream>
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
