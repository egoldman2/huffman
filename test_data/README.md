# Real-world compression inputs

Downloaded on 2026-09-27. These are original input files, not benchmark results.
The six samples total 26,013,319 bytes (about 24.8 MiB). Keep downloaded files
unchanged so later measurements use the same inputs.

| File | Bytes | Why test it | Source |
|---|---:|---|---|
| `pride_and_prejudice.txt` | 772,386 | Natural-language prose and UTF-8 characters | [Project Gutenberg](https://www.gutenberg.org/ebooks/1342.txt.utf-8) |
| `gtest.cc` | 266,604 | Real C++ code with repeated identifiers, whitespace and comments | [GoogleTest v1.17.0](https://raw.githubusercontent.com/google/googletest/v1.17.0/googletest/src/gtest.cc) |
| `rfc1951.txt` | 36,944 | Technical text with tables and formatting | [RFC Editor](https://www.rfc-editor.org/rfc/rfc1951.txt) |
| `rfc1951.pdf` | 56,620 | The same technical document in a binary document format | [RFC Editor](https://www.rfc-editor.org/rfc/rfc1951.pdf) |
| `python-logo.png` | 45,187 | A real image in a compressed format | [Python Software Foundation](https://www.python.org/static/community_logos/python-logo.png) |
| `pride_and_prejudice.epub` | 24,835,578 | A larger, already-compressed ebook containing images; contrasts with the text edition | [Project Gutenberg](https://www.gutenberg.org/ebooks/1342.epub3.images) |

These examples cover different byte distributions and existing compression.
They do not cover every workload. Add synthetic empty, tiny, repeated-byte and
seeded random inputs separately when benchmarking, and label them synthetic.

## Trying a sample

From the project folder, run `./huff`, choose `1`, and enter
`test_data/pride_and_prejudice.txt`. Accept the default archive path.
Then choose `2` with `test_data/pride_and_prejudice.txt.huf`, and use a different
output path such as `test_data/restored.txt` so the original stays untouched.

Verify restoration with:

```sh
cmp test_data/pride_and_prejudice.txt test_data/restored.txt
```

No output and exit status 0 mean the files match exactly. File signatures and
sizes have been checked. The initial harness run verified exact restoration
for both tools on all six samples over three repetitions; its raw measurements
are in `bench/results.csv`. Keep generated archives and restored files out of
the original-input list.

## Comparing against gzip

Run `python3 bench/compare.py` from the project folder. The harness creates
Huffman and gzip archives in a temporary directory, measures both tools, checks
exact restoration and removes the generated copies. See
[the benchmark instructions](../bench/README.md). No precompressed dataset copies
are needed.

## Original SHA-256 checksums

```text
59ee4e6ee5d637510633db22fecbec929644a406d5c581ac5df53de4bab1c507  gtest.cc
bbd82efa5e3e8d8a302a39c5e20d7d6d250804c7003c63378946d85f1176ccb6  pride_and_prejudice.epub
3f6bb9d6f78e0293b56acd4714dd68cb7d6d1d293402031ce9d5a216bcaf9d75  pride_and_prejudice.txt
ea0e73137c1c8561e91241771e3c81e5b3e8f5ab2cbfdad1a00d8eea524815fa  python-logo.png
44ebfc2a0af072f08bc96d68f5f9193ec41332a9e01a46f33b1834d85caced6e  rfc1951.pdf
5ebf4b5b7fe1c3a0c0ab9aa3ac8c0f3853a7dc484905e76e03b0b0f301350009  rfc1951.txt
```

Preserve the copyright/license notices included in the downloaded text and
source code. The Python logo remains subject to the PSF's
[logo usage policy](https://www.python.org/community/logos/); it is included as
an unchanged test input, not project branding.
