# Huffman File Compressor

A C++17 project for **Programming Assignment 1 — Track B: Implementation and Building Something**. Implement Huffman coding and use it to build a lossless file compressor with a basic interactive command-line interface.

**Status:** file compression, decompression and archive inspection work through the menu, with overwrite confirmation and temporary-output protection. Benchmarks and submission documentation remain unfinished.

## Build and run

Compile and run directly with a C++17 compiler:

```sh
c++ -std=c++17 -Isrc src/main.cpp src/huffman.cpp src/tree_io.cpp src/archive.cpp src/file_ops.cpp -o huff
./huff
```

The program opens in the terminal's alternate screen and restores the previous terminal contents when it exits. Choose `1` to compress or `2` to decompress, enter an input path, and press Enter to accept the output default or enter another path. Results stay visible until you press Enter to return to the menu. Choose `0` or send end-of-input to exit.

The tests use GoogleTest. Install it once:

```sh
brew install googletest
```

Then, from this folder, compile and run the tests with one command:

```sh
c++ -std=c++17 -Isrc -I"$(brew --prefix googletest)/include" tests/test_huffman.cpp tests/test_archive.cpp tests/test_file_ops.cpp src/huffman.cpp src/tree_io.cpp src/archive.cpp src/file_ops.cpp -L"$(brew --prefix googletest)/lib" -lgtest_main -lgtest -pthread -o huffman_tests && ./huffman_tests
```

GoogleTest runs every test even when one fails and prints the failure reason, time per test and total time automatically.

## Interface

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

Compression and decompression prompt for input and output paths. Option `3` prompts for an archive path and displays its format, original/archive sizes, symbol count, header/tree size, payload size and compression statistics. After completion or a recoverable error, press Enter to return to the menu.

Inspection validates the full archive through the decoder, discarding restored bytes without creating an output file. HUF1 has no checksum: corruption that still decodes into the expected number of bytes may go undetected. Inspection time therefore depends on the payload size.

### Example interaction

```text
Choose an option: 1
Input file: notes.txt
Output file [notes.txt.huf]:

Compression complete.
Saved to: notes.txt.huf
Original size, archive size, compression ratio and percentage saved are displayed.
```

Press Enter to accept a displayed output default. A blank input path cancels the operation. Paths containing spaces are accepted without shell quoting. Selecting `0` or reaching end-of-input exits cleanly.

Existing outputs require confirmation before replacement. The source must never be overwritten by its own operation.

Output symbolic links are rejected. Operations write beside the destination in a temporary directory and rename the completed file into place. On failure, temporary output is removed and an existing destination is preserved.

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
