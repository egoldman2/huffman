# Huffman vs gzip

From the project folder:

```sh
python3 bench/compare.py
```

This compiles the small Huffman driver with C++17 and `-O2`, tests the six
original files in `test_data`, and runs each tool three times. It prints complete
archive sizes, percentage saved, median compression/decompression times and exact
restoration checks. Raw measurements and environment information go into
`bench/results.csv`. Existing result files are never overwritten.

To test selected files or save a separate run:

```sh
python3 bench/compare.py test_data/rfc1951.txt test_data/python-logo.png --runs 5 --output bench/second_run.csv
```

Both tools are timed as subprocesses performing file-to-file operations,
including process startup, file opening and output close. Input generation,
compilation and byte comparison are outside timing. Gzip uses explicit level 6
and omits original names/timestamps. Huffman uses the same file helpers as the
menu, including temporary-output creation and final rename; gzip writes directly
to a temporary output file. These are end-to-end tool timings, not isolated
algorithm timings. No explicit fsync is performed.

Tool order alternates across repetitions, but filesystem caches are not flushed.
Small-file times can be dominated by startup and measurement noise. There is no
warm-up run. Empty-input savings are N/A. Temporary archives and restored copies
are removed automatically; the original samples are never modified.
