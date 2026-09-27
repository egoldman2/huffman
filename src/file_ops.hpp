#pragma once

#include "archive.hpp"

#include <filesystem>

namespace huffman {

    // Existing outputs are replaced only when overwrite is explicitly true
    ArchiveHeader compress_file(
        const std::filesystem::path& source,
        const std::filesystem::path& destination,
        bool overwrite = false
    );

    ArchiveHeader decompress_file(
        const std::filesystem::path& source,
        const std::filesystem::path& destination,
        bool overwrite = false
    );

}
