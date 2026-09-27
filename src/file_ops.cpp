#include "file_ops.hpp"

#include <fstream>
#include <random>
#include <stdexcept>
#include <string>
#include <streambuf>

namespace huffman {

    namespace {
        // let the existing decoder validate bytes without saving them anywhere
        class DiscardBuffer : public std::streambuf {
            int_type overflow(int_type byte) override {
                return traits_type::not_eof(byte);
            }
        };

        void check_destination(
            const std::filesystem::path& source,
            const std::filesystem::path& destination,
            bool overwrite
        ) {
            namespace fs = std::filesystem;

            if (fs::is_symlink(destination)) {
                throw std::runtime_error("output must not be a symbolic link");
            }
            if (fs::exists(destination)) {
                if (fs::equivalent(source, destination)) {
                    throw std::runtime_error("input and output must be different files");
                }
                if (!fs::is_regular_file(destination)) {
                    throw std::runtime_error("output must be a regular file");
                }
                if (!overwrite) {
                    throw std::runtime_error("output already exists; replacement was not approved");
                }
            }
        }

        ArchiveHeader process_file(
            const std::filesystem::path& source,
            const std::filesystem::path& destination,
            bool overwrite,
            bool decompressing
        ) {
            namespace fs = std::filesystem;

            if (!fs::is_regular_file(source)) {
                throw std::runtime_error("input must be an existing regular file");
            }
            check_destination(source, destination, overwrite);

            std::ifstream input(source, std::ios::binary);
            if (!input) {
                throw std::runtime_error("could not open the input file");
            }

            // create a private temporary directory beside the destination
            // keeping it on the same filesystem lets rename finalize the output
            std::random_device random;
            fs::path temporary_directory;
            bool created = false;
            for (int attempt = 0; attempt < 16; ++attempt) {
                temporary_directory = destination.parent_path() /
                    (".huff-tmp-" + std::to_string(random()));
                if (fs::create_directory(temporary_directory)) {
                    created = true;
                    break;
                }
            }
            if (!created) {
                throw std::runtime_error("could not create temporary output");
            }

            const auto temporary_file = temporary_directory / "output";
            try {
                std::ofstream output(temporary_file, std::ios::binary);
                if (!output) {
                    throw std::runtime_error("could not open temporary output");
                }

                const auto header = decompressing ? decompress(input, output) : compress(input, output);
                output.close();
                if (!output) {
                    throw std::runtime_error("could not finish writing the output file");
                }
                input.close();

                // recheck before replacing anything, then publish the completed file
                check_destination(source, destination, overwrite);
                fs::rename(temporary_file, destination);
                std::error_code ignored;
                fs::remove(temporary_directory, ignored);
                return header;
            } catch (...) {
                // remove only the temporary data created by this operation
                std::error_code ignored;
                fs::remove(temporary_file, ignored);
                fs::remove(temporary_directory, ignored);
                throw;
            }
        }
    }

    ArchiveHeader compress_file(
        const std::filesystem::path& source,
        const std::filesystem::path& destination,
        bool overwrite
    ) {
        return process_file(source, destination, overwrite, false);
    }

    ArchiveHeader decompress_file(
        const std::filesystem::path& source,
        const std::filesystem::path& destination,
        bool overwrite
    ) {
        return process_file(source, destination, overwrite, true);
    }

    ArchiveHeader inspect_file(const std::filesystem::path& source) {
        if (!std::filesystem::is_regular_file(source)) {
            throw std::runtime_error("input must be an existing regular file");
        }
        std::ifstream input(source, std::ios::binary);
        if (!input) {
            throw std::runtime_error("could not open the archive");
        }

        DiscardBuffer buffer;
        std::ostream discarded_output(&buffer);
        return decompress(input, discarded_output);
    }

}
