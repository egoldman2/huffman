#include "huffman.hpp"

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {

using Clock = std::chrono::steady_clock;

void expect(bool condition, const std::string& description) {
    if (!condition) {
        std::cerr << "[FAIL] " << description << '\n';
        std::exit(EXIT_FAILURE);
    }

    std::cout << "[PASS] " << description << '\n';
}

void test_frequency_counting() {
    const std::string bytes = {'A', '\0', 'A', static_cast<char>(0xFF)};
    std::istringstream input(bytes);
    const auto frequencies = huffman::count_frequencies(input);

    expect(frequencies[static_cast<unsigned char>('A')] == 2,
           "counts repeated ASCII bytes");
    expect(frequencies[0x00] == 1,
           "counts the NUL byte");
    expect(frequencies[0xFF] == 1,
           "counts byte values above 127");
    expect(frequencies[static_cast<unsigned char>('B')] == 0,
           "leaves missing bytes at zero");
}

void test_empty_tree() {
    const huffman::FrequencyTable empty_frequencies{};
    expect(huffman::create_tree(empty_frequencies).empty(),
           "creates an empty tree from empty input");
}

void test_single_symbol_tree() {
    huffman::FrequencyTable single_frequency{};
    single_frequency[static_cast<unsigned char>('X')] = 4;
    const auto single_tree = huffman::create_tree(single_frequency);

    expect(single_tree.size() == 1,
           "keeps one symbol as one node");
    expect(single_tree.back().frequency == 4,
           "preserves a single symbol's frequency");
    expect(single_tree.back().symbol == 'X',
           "preserves a single symbol's byte value");
    expect(single_tree.back().left == -1 && single_tree.back().right == -1,
           "gives a leaf no children");
}

void test_banana_tree() {
    huffman::FrequencyTable banana_frequencies{};
    banana_frequencies[static_cast<unsigned char>('A')] = 3;
    banana_frequencies[static_cast<unsigned char>('N')] = 2;
    banana_frequencies[static_cast<unsigned char>('B')] = 1;
    const auto banana_tree = huffman::create_tree(banana_frequencies);

    expect(banana_tree.size() == 5,
           "creates three leaves and two parents for BANANA");

    const auto& root = banana_tree.back();
    expect(root.frequency == 6,
           "stores the total frequency at the root");
    expect(root.symbol == -1,
           "marks the root as an internal node");
    expect(root.left >= 0 && root.right >= 0 &&
               static_cast<std::size_t>(root.left) < banana_tree.size() &&
               static_cast<std::size_t>(root.right) < banana_tree.size(),
           "stores valid child indexes at the root");
    expect(banana_tree[root.left].frequency + banana_tree[root.right].frequency ==
               root.frequency,
           "makes the root frequency equal its children's sum");
}

void run_test(const std::string& name, void (*test)()) {
    std::cout << "\n" << name << '\n';

    const auto start = Clock::now();
    test();
    const auto elapsed = std::chrono::duration<double, std::milli>(
        Clock::now() - start);

    std::cout << "[TIME] " << std::fixed << std::setprecision(3)
              << elapsed.count() << " ms\n";
}

}  // namespace

int main() {
    const auto total_start = Clock::now();

    std::cout << "Huffman tests\n";

    run_test("Frequency counting", test_frequency_counting);
    run_test("Empty tree", test_empty_tree);
    run_test("Single-symbol tree", test_single_symbol_tree);
    run_test("BANANA tree", test_banana_tree);

    const auto total_elapsed = std::chrono::duration<double, std::milli>(
        Clock::now() - total_start);

    std::cout << "\nAll tests passed.\n"
              << "[TOTAL] " << std::fixed << std::setprecision(3)
              << total_elapsed.count() << " ms\n";
}
