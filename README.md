# Huffman File Compressor

A C++17 project for **Programming Assignment 1 — Track B: Implementation and Building Something**. Implement Huffman coding and use it to build a lossless file compressor with a basic interactive command-line interface.

**Status:** planning only. The executable, build configuration and tests have not been implemented. The interface below describes the intended behavior.

## Run without arguments

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

The program prompts for input and output paths after an operation is selected. After completion or a recoverable error, it returns to the menu.

### Example interaction

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

## Build and tests

A C++17 compiler will be required. Exact build, test and benchmark commands will be added and verified once implementation exists.

## Documentation

See [the full project plan](PLAN.md) for design decisions, all implementation checkpoints, testing, gzip evaluation, report requirements, the video and the submission checklist.

The final submission includes the working repository, interactive tool, written report and a 3–5 minute video walkthrough.
