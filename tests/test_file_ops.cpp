#include "file_ops.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <stdexcept>
#include <string>

namespace {

class FileOperations : public ::testing::Test {
protected:
    std::filesystem::path directory;
    bool owns_directory = false;

    void SetUp() override {
        std::random_device random;
        directory = std::filesystem::temp_directory_path() /
            ("huffman-test-" + std::to_string(random()));
        owns_directory = std::filesystem::create_directory(directory);
        ASSERT_TRUE(owns_directory);
    }

    void TearDown() override {
        // this directory contains only files created by this test
        std::error_code ignored;
        if (owns_directory) {
            std::filesystem::remove_all(directory, ignored);
        }
    }

    void write(const std::filesystem::path& path, const std::string& bytes) {
        std::ofstream output(path, std::ios::binary);
        output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        output.close();
        ASSERT_TRUE(output);
    }

    std::string read(const std::filesystem::path& path) {
        std::ifstream input(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(input), {});
    }

    void expect_no_temporary_files() {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            EXPECT_NE(entry.path().filename().string().find(".huff-tmp-"), std::size_t{0});
        }
    }
};

TEST_F(FileOperations, RestoresBinaryFilesWithSpacesInTheirPaths) {
    const auto source = directory / "original bytes.bin";
    const auto archive = directory / "compressed bytes.huf";
    const auto restored = directory / "restored bytes.bin";
    std::string original;
    for (int symbol = 0; symbol < 256; ++symbol) {
        original += static_cast<char>(symbol);
    }
    write(source, original);

    huffman::compress_file(source, archive);
    huffman::decompress_file(archive, restored);

    EXPECT_EQ(read(source), original);
    EXPECT_EQ(read(restored), original);
    expect_no_temporary_files();
}

TEST_F(FileOperations, RequiresApprovalBeforeReplacingAnOutput) {
    const auto source = directory / "input";
    const auto destination = directory / "output";
    write(source, "BANANA");
    write(destination, "keep this");

    EXPECT_THROW(huffman::compress_file(source, destination), std::runtime_error);
    EXPECT_EQ(read(destination), "keep this");
    huffman::compress_file(source, destination, true);
    EXPECT_EQ(read(destination).substr(0, 4), "HUF1");
    expect_no_temporary_files();
}

TEST_F(FileOperations, RejectsSameFileHardLinksAndSymbolicLinks) {
    const auto source = directory / "input";
    const auto hard_link = directory / "hard link";
    const auto symbolic_link = directory / "symbolic link";
    write(source, "BANANA");
    std::filesystem::create_hard_link(source, hard_link);
    std::filesystem::create_symlink(source, symbolic_link);

    EXPECT_THROW(huffman::compress_file(source, source, true), std::runtime_error);
    EXPECT_THROW(huffman::compress_file(source, hard_link, true), std::runtime_error);
    EXPECT_THROW(huffman::compress_file(source, symbolic_link, true), std::runtime_error);
    EXPECT_EQ(read(source), "BANANA");
}

TEST_F(FileOperations, PreservesExistingOutputAndCleansUpAfterInvalidArchives) {
    const auto source = directory / "invalid.huf";
    const auto destination = directory / "output";
    write(source, "bad archive");
    write(destination, "keep this");

    EXPECT_THROW(huffman::decompress_file(source, destination, true), std::runtime_error);
    EXPECT_EQ(read(destination), "keep this");
    expect_no_temporary_files();
}

TEST_F(FileOperations, RemovesPartialRestorationAfterTruncatedPayload) {
    const auto source = directory / "input";
    const auto archive = directory / "truncated.huf";
    const auto destination = directory / "output";
    write(source, "BANANA");
    huffman::compress_file(source, archive);
    auto bytes = read(archive);
    bytes.pop_back();
    write(archive, bytes);
    write(destination, "keep this");

    EXPECT_THROW(huffman::decompress_file(archive, destination, true), std::runtime_error);
    EXPECT_EQ(read(destination), "keep this");
    expect_no_temporary_files();
}

TEST_F(FileOperations, InspectsMetadataWithoutChangingOrExtractingFiles) {
    const auto source = directory / "input";
    const auto archive = directory / "input.huf";
    write(source, "BANANA");
    huffman::compress_file(source, archive);
    const auto before = read(archive);

    const auto header = huffman::inspect_file(archive);

    EXPECT_EQ(header.original_size, std::uint64_t{6});
    EXPECT_EQ(header.symbol_count, std::uint16_t{3});
    EXPECT_EQ(header.bit_count, std::uint64_t{9});
    EXPECT_EQ(std::filesystem::file_size(archive), std::uintmax_t{32});
    EXPECT_EQ(read(archive), before);
    EXPECT_EQ(std::distance(std::filesystem::directory_iterator(directory),
                            std::filesystem::directory_iterator{}), 2);
}

TEST_F(FileOperations, InspectsEmptyArchives) {
    const auto source = directory / "empty";
    const auto archive = directory / "empty.huf";
    write(source, "");
    huffman::compress_file(source, archive);
    const auto header = huffman::inspect_file(archive);

    EXPECT_EQ(header.original_size, std::uint64_t{0});
    EXPECT_EQ(header.symbol_count, std::uint16_t{0});
    EXPECT_EQ(header.bit_count, std::uint64_t{0});
    EXPECT_EQ(std::filesystem::file_size(archive), std::uintmax_t{22});
}

TEST_F(FileOperations, InspectionRejectsMissingFilesAndMalformedPayloads) {
    const auto source = directory / "input";
    const auto archive = directory / "input.huf";
    EXPECT_THROW(huffman::inspect_file(archive), std::runtime_error);
    write(source, "BANANA");
    huffman::compress_file(source, archive);
    auto bytes = read(archive);
    bytes.pop_back();
    write(archive, bytes);

    EXPECT_THROW(huffman::inspect_file(archive), std::runtime_error);
    EXPECT_EQ(read(archive), bytes);
    expect_no_temporary_files();
}

TEST_F(FileOperations, RejectsMissingInputAndInvalidDestinations) {
    const auto source = directory / "input";
    const auto destination = directory / "output";
    EXPECT_THROW(huffman::compress_file(source, destination), std::runtime_error);
    EXPECT_FALSE(std::filesystem::exists(destination));

    write(source, "BANANA");
    EXPECT_THROW(huffman::compress_file(source, directory, true), std::runtime_error);
    EXPECT_THROW(huffman::compress_file(source, directory / "missing" / "output"),
                 std::filesystem::filesystem_error);
    EXPECT_EQ(read(source), "BANANA");
}

}
