#include <iostream>
#include <string>

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

        if (choice == "4") {
            std::cout << "\nThis is an experimental scaffold; file operations are not implemented yet.\n\n";
        } else if (choice == "1" || choice == "2" || choice == "3") {
            // Hint: prompts belong here; Huffman logic belongs in huffman.cpp.
            std::cout << "\nNot implemented yet.\n\n";
        } else {
            std::cout << "\nPlease choose 0, 1, 2, 3, or 4.\n\n";
        }
    }
}
