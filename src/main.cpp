#include "file_ops.hpp"

#include <chrono>
#include <filesystem>
#include <future>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>

namespace {

void enter_app_screen() {
    std::cout << "\033[?1049h" << std::flush;
}

void leave_app_screen() {
    std::cout << "\033[?1049l" << std::flush;
}

void clear_terminal() {
    std::cout << "\033[3J\033[2J\033[H" << std::flush;
}

void print_menu() {
    std::cout << "Huffman File Compressor\n\n"
              << "1. Compress a file\n"
              << "2. Decompress a file\n"
              << "3. Inspect a compressed file\n"
              << "4. Help\n"
              << "0. Exit\n\n"
              << "Choose an option: ";
}

void print_archive_statistics(const huffman::ArchiveHeader& header, std::uintmax_t archive_size) {
    const auto payload_bytes = header.bit_count / 8 + (header.bit_count % 8 != 0);
    const auto tree_bytes = header.symbol_count == 0 ? 0 : 3 * header.symbol_count - 1;

    std::cout << "Format: HUF1\n"
              << "Original size: " << header.original_size << " bytes\n"
              << "Archive size: " << archive_size << " bytes\n"
              << "Symbols: " << header.symbol_count << '\n'
              << "Header and tree: " << 22 + tree_bytes << " bytes\n"
              << "Payload: " << header.bit_count << " bits (" << payload_bytes << " bytes)\n";

    if (header.original_size != 0) {
        const double ratio = static_cast<double>(archive_size) / header.original_size;
        std::cout << std::fixed << std::setprecision(2)
                  << "Compression ratio: " << ratio << '\n'
                  << "Space saved: " << 100 * (1 - ratio) << "%\n";
    } else {
        std::cout << "Compression ratio: N/A (empty input)\n"
                  << "Space saved: N/A (empty input)\n";
    }
}

bool inspect_archive() {
    std::cout << "Archive file (blank to cancel): ";
    std::string source;
    if (!std::getline(std::cin, source)) {
        return false;
    }
    if (source.empty()) {
        std::cout << "Cancelled.\n";
        return true;
    }

    const auto header = huffman::inspect_file(source);
    print_archive_statistics(header, std::filesystem::file_size(source));
    std::cout << "Archive structure and payload validated; no file was extracted.\n";
    return true;
}

bool run_file_operation(bool decompressing) {
    std::cout << "Input file (blank to cancel): ";
    std::string source;
    if (!std::getline(std::cin, source)) {
        return false;
    }
    if (source.empty()) {
        std::cout << "Cancelled.\n";
        return true;
    }

    std::string default_output = source + ".huf";
    if (decompressing) {
        if (source.size() > 4 && source.substr(source.size() - 4) == ".huf") {
            default_output = source.substr(0, source.size() - 4);
        } else {
            default_output = source + ".out";
        }
    }

    std::cout << "Output file [" << default_output << "]: ";
    std::string destination;
    if (!std::getline(std::cin, destination)) {
        return false;
    }
    if (destination.empty()) {
        destination = default_output;
    }

    bool overwrite = false;
    if (std::filesystem::exists(destination)) {
        std::cout << "Output exists. Replace it? [y/N]: ";
        std::string confirmation;
        if (!std::getline(std::cin, confirmation)) {
            return false;
        }
        overwrite = confirmation == "y" || confirmation == "Y";
        if (!overwrite) {
            std::cout << "Cancelled.\n";
            return true;
        }
    }

    // run the file operation in the background so the menu can display elapsed time
    using Clock = std::chrono::steady_clock;
    const auto start = Clock::now();
    auto operation = std::async(std::launch::async, [=] {
        const auto header = decompressing
            ? huffman::decompress_file(source, destination, overwrite)
            : huffman::compress_file(source, destination, overwrite);
        const double seconds = std::chrono::duration<double>(Clock::now() - start).count();
        return std::make_pair(header, seconds);
    });

    const char* action = decompressing ? "Decompressing" : "Compressing";
    std::cout << action << "... elapsed: 0.00 s" << std::flush;
    while (operation.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready) {
        const double seconds = std::chrono::duration<double>(Clock::now() - start).count();
        std::cout << "\r\033[2K" << action << "... elapsed: "
                  << std::fixed << std::setprecision(2) << seconds << " s" << std::flush;
    }

    // clear the live timer before printing results or an error
    std::cout << "\r\033[2K" << std::flush;
    const auto [header, seconds] = operation.get();

    std::cout << (decompressing ? "Decompression complete.\n" : "Compression complete.\n")
              << "Saved to: " << destination << '\n';
    if (!decompressing) {
        print_archive_statistics(header, std::filesystem::file_size(destination));
    } else {
        std::cout << "Original size: " << header.original_size << " bytes\n";
    }
    std::cout << "Time taken: " << std::fixed << std::setprecision(3) << seconds << " s\n";

    return true;
}

}  // namespace

int main() {
    enter_app_screen();
    clear_terminal();

    for (;;) {
        print_menu();

        std::string choice;
        if (!std::getline(std::cin, choice)) {
            leave_app_screen();
            return 0;
        }

        clear_terminal();

        if (choice == "0") {
            leave_app_screen();
            return 0;
        }

        try {
            if (choice == "1" || choice == "2") {
                if (!run_file_operation(choice == "2")) {
                    leave_app_screen();
                    return 0;
                }
            } else if (choice == "4") {
                std::cout << "Compress creates a .huf archive; decompress restores its original bytes.\n"
                          << "Enter paths directly, including spaces, without surrounding quotes.\n"
                          << "Blank input cancels; blank output accepts the displayed default.\n"
                          << "Existing outputs require confirmation. Small files may grow.\n"
                          << "Inspect validates an archive and shows its sizes without extracting a file.\n"
                          << "HUF1 has no checksum; some payload corruption may go undetected.\n";
            } else if (choice == "3") {
                if (!inspect_archive()) {
                    leave_app_screen();
                    return 0;
                }
            } else {
                std::cout << "Please choose 0, 1, 2, 3, or 4.\n";
            }
        } catch (const std::exception& error) {
            std::cout << "Error: " << error.what() << '\n';
        }

        std::cout << "\nPress Enter to return to the menu...";
        std::string pause;
        if (!std::getline(std::cin, pause)) {
            leave_app_screen();
            return 0;
        }
        clear_terminal();
    }
}
