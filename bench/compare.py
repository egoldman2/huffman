"""Compare complete file operations; compile once, time runs, then check bytes."""

import argparse
import csv
import filecmp
import platform
import shutil
import statistics
import subprocess
import tempfile
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SAMPLES = [
    "pride_and_prejudice.txt", "gtest.cc", "rfc1951.txt",
    "rfc1951.pdf", "python-logo.png", "pride_and_prejudice.epub",
]


def timed_run(command, output=None):
    start = time.perf_counter()
    if output is None:
        subprocess.run(command, check=True)
    else:
        with output.open("wb") as destination:
            subprocess.run(command, stdout=destination, check=True)
    return time.perf_counter() - start


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("files", nargs="*", type=Path, help="optional input files")
    parser.add_argument("--runs", type=int, default=3, help="repetitions per tool (default: 3)")
    parser.add_argument("--output", type=Path, default=ROOT / "bench/results.csv")
    args = parser.parse_args()
    if args.runs < 1:
        parser.error("--runs must be at least 1")

    compiler = shutil.which("c++")
    gzip = shutil.which("gzip")
    if not compiler or not gzip:
        parser.error("c++ and gzip must be installed and available on PATH")
    files = [path.resolve() for path in args.files] if args.files else [
        ROOT / "test_data" / name for name in SAMPLES
    ]
    for source in files:
        if not source.is_file():
            parser.error(f"input does not exist or is not a file: {source}")
    result_path = args.output.resolve()
    if result_path in files:
        parser.error("results must not overwrite an input file")
    if result_path.exists():
        parser.error(f"results already exist; choose a new --output path: {result_path}")

    compiler_version = subprocess.check_output([compiler, "--version"], text=True).splitlines()[0]
    gzip_version = subprocess.check_output(
        [gzip, "--version"], text=True, stderr=subprocess.STDOUT
    ).splitlines()[0]
    machine = platform.platform()
    print(f"Compiler: {compiler_version}; flags: -std=c++17 -O2")
    print(f"Gzip: {gzip_version}; flags: -n -6 -c (compress), -d -c (decompress)")
    print(f"Machine: {machine}; repetitions: {args.runs}")
    print("Times include process startup and completed file I/O; checks are outside timing.\n")

    rows = []
    with tempfile.TemporaryDirectory(prefix="huffman-bench-") as temporary:
        workspace = Path(temporary)
        driver = workspace / "driver"
        sources = ["huffman.cpp", "tree_io.cpp", "archive.cpp", "file_ops.cpp"]
        subprocess.run([
            compiler, "-std=c++17", "-O2", "-I", str(ROOT / "src"),
            str(ROOT / "bench/driver.cpp"),
            *[str(ROOT / "src" / name) for name in sources], "-o", str(driver),
        ], check=True)

        for index, source in enumerate(files):
            original_bytes = source.stat().st_size
            case = workspace / str(index)
            case.mkdir()
            print(f"{source.name} ({original_bytes:,} original bytes)", flush=True)
            measurements = {"huffman": [], "gzip": []}
            for run in range(1, args.runs + 1):
                # alternate order to reduce consistently favoring one tool's cache state
                tools = ["huffman", "gzip"] if run % 2 else ["gzip", "huffman"]
                for tool in tools:
                    archive = case / f"{tool}-{run}.archive"
                    restored = case / f"{tool}-{run}.restored"
                    if tool == "huffman":
                        compress_s = timed_run([str(driver), "compress", str(source), str(archive)])
                        decompress_s = timed_run([str(driver), "decompress", str(archive), str(restored)])
                    else:
                        compress_s = timed_run([gzip, "-n", "-6", "-c", str(source)], archive)
                        decompress_s = timed_run([gzip, "-d", "-c", str(archive)], restored)

                    if not filecmp.cmp(source, restored, shallow=False):
                        raise RuntimeError(f"{tool} restoration failed: {source}")
                    archive_bytes = archive.stat().st_size
                    saved_pct = 100 * (1 - archive_bytes / original_bytes) if original_bytes else None
                    rows.append({
                        "file": str(source), "tool": tool, "run": run,
                        "original_bytes": original_bytes, "archive_bytes": archive_bytes,
                        "saved_pct": saved_pct, "compress_s": compress_s,
                        "decompress_s": decompress_s, "restored_exactly": True,
                        "compiler": compiler_version, "compiler_flags": "-std=c++17 -O2",
                        "gzip": gzip_version, "gzip_flags": "-n -6 -c / -d -c",
                        "machine": machine,
                    })
                    measurements[tool].append((archive_bytes, compress_s, decompress_s, saved_pct))
                    # archives and restored copies are temporary, not new dataset inputs
                    archive.unlink()
                    restored.unlink()

            for tool, runs in measurements.items():
                size = runs[0][0]
                saved = f"{runs[0][3]:.2f}%" if original_bytes else "N/A"
                compress_ms = statistics.median(row[1] for row in runs) * 1000
                decompress_ms = statistics.median(row[2] for row in runs) * 1000
                print(f"  {tool:7} {size:>10,} bytes | saved {saved:>8} | "
                      f"compress {compress_ms:8.2f} ms | decompress {decompress_ms:8.2f} ms | exact OK")

    with result_path.open("x", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    print(f"\nRaw runs saved to {result_path}")


if __name__ == "__main__":
    main()
