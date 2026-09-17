# Huffman File Compressor

A C++17 project for **Programming Assignment 1 — Track B: Implementation and Building Something**. Implement Huffman coding and use it to build a lossless file compressor with a basic interactive command-line interface.

**Status:** experimental scaffold. The menu and byte-frequency counter are in place; compression, decompression and archive handling are not implemented yet.

## Current scaffold

Compile and run directly with a C++17 compiler:

```sh
c++ -std=c++17 -Isrc src/main.cpp src/huffman.cpp -o huff
./huff
```

The program opens in the terminal's alternate screen and restores the previous terminal contents when it exits. It currently displays the planned menu, handles invalid choices and exits with `0` or end-of-input. Compression, decompression and inspection choices are placeholders.

The tests use GoogleTest. Install it once:

```sh
brew install googletest
```

Then, from this folder, compile and run the tests with one command:

```sh
c++ -std=c++17 -Isrc -I"$(brew --prefix googletest)/include" tests/test_huffman.cpp src/huffman.cpp -L"$(brew --prefix googletest)/lib" -lgtest_main -lgtest -pthread -o huffman_tests && ./huffman_tests
```

GoogleTest runs every test even when one fails and prints the failure reason, time per test and total time automatically.

## Planned interface

```sh
./huff
```

This opens the menu:

```text
Huffman File Compressor

1. Compress a file
2. Decompress a file
3. Inspect a compressed file
4. Help
0. Exit

Choose an option:
```

The completed program will prompt for input and output paths after an operation is selected. After completion or a recoverable error, it will return to the menu.

### Planned example interaction

```text
Choose an option: 1
Input file: notes.txt
Output file [notes.txt.huf]:

Compression complete.
Saved to: notes.txt.huf
[Original size, archive size and space saved shown here]
```

Press Enter to accept a displayed output default. A blank input path cancels the operation. Paths containing spaces are accepted without shell quoting. Selecting `0` or reaching end-of-input exits cleanly.

Existing outputs require confirmation before replacement. The source must never be overwritten by its own operation.

## Planned features

- Compress and decompress arbitrary binary files, including text and UTF-8.
- Restore the exact original bytes.
- Store the Huffman tree and decoding metadata inside each archive.
- Pack encoded bits into bytes.
- Show original size, archive size, compression ratio and percentage saved.
- Inspect archive metadata without extracting its contents.
- Handle empty files, single-symbol files, invalid input and malformed archives.
- Preserve inputs and clean up incomplete output after failures.

Compression can increase file size when metadata outweighs payload savings. Statistics will report expansion honestly.

## Approach

Count byte frequencies, repeatedly merge the two least frequent trees, and assign codes from root-to-leaf paths. Write the tree and packed payload into the archive. Decompression reconstructs the tree and follows the bits to recover the original bytes.

Use the C++ standard library, including a minimum-priority queue. The menu calls reusable compression and decompression functions so those operations can also be tested and benchmarked directly.

## Documentation

See [the full project plan](PLAN.md) for design decisions, all implementation checkpoints, testing, gzip evaluation, report requirements, the video and the submission checklist.

The final submission includes the working repository, interactive tool, written report and a 3–5 minute video walkthrough.
