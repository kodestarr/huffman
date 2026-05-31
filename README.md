# Huffman Compression Tool

A cross-platform command-line file compression utility implementing adaptive Huffman coding with support for single files, recursive directories, and wildcard pattern matching.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey.svg)
![Language](https://img.shields.io/badge/language-C-orange.svg)

---

## Features

- **Single File Compression/Decompression**
  - Compress any file using adaptive Huffman coding
  - Auto-detects compressed files by magic header for seamless decompression

- **Recursive Directory Compression/Decompression**
  - Compress entire folders including all subdirectories
  - Preserves complete directory structure in the archive

- **Wildcard Pattern Matching**
  - Batch-select files using `*` and `?` wildcards
  - Flexible file selection for targeted compression

- **Progress Animation**
  - Optional real-time progress bar with compression ratio display
  - Toggle on/off based on preference

---

## Building

### Prerequisites

- C compiler (GCC, MinGW, or MSVC)
- Make (optional, for Makefile builds)

### Compile

```bash
gcc -o huffman compress.c decompress.c main.c
```

For Windows with MinGW:
```bash
gcc -o huffman.exe compress.c decompress.c main.c
```

---

## Usage

Launch the interactive mode:

```bash
./huffman
```

### Interactive Commands

| Step | Input | Description |
|------|-------|-------------|
| Source | `file.txt` / `folder/` / `*.txt` | File, directory, or wildcard pattern |
| Destination | `output.bin` | Output archive path |
| Progress | `y` / `n` | Enable/disable progress animation |

### Wildcard Patterns

| Pattern | Matches |
|---------|---------|
| `*.txt` | All `.txt` files in current directory |
| `report?.doc` | `report1.doc`, `reportA.doc`, etc. |
| `data/*.csv` | All `.csv` files in `data/` folder |
| `**/*.log` | All `.log` files recursively |

---

## File Format

```
[magic header: 33 bytes]
[file count: 8 bytes]          (only for multi-file archives)
[
  [name length: 1 byte]
  [file name: N bytes]
  [compressed length: 8 bytes]
  [compressed data: N bytes]
] * file count
```

Magic header: `iuc-aic,ae=vb;abvj?bjhbb'vrb%vb*ab`

---

## Project Structure

```
.
├── huffman.h       # Header file with type definitions and function declarations
├── main.c          # Interactive CLI interface and wildcard expansion
├── compress.c      # Huffman encoding, file collection, and archive generation
├── decompress.c    # Huffman decoding and archive extraction
└── README.md       # This file
```

---

## Algorithm

### Huffman Coding

1. **Frequency Analysis**: Scan input file to build byte frequency table
2. **Tree Construction**: Build minimum-heap based Huffman tree
3. **Canonical Coding**: Generate length-limited canonical Huffman codes
4. **Encoding**: Replace each byte with its variable-length code
5. **Padding**: Pad final byte to maintain byte alignment

### Compression Flow

```
Input File → Frequency Table → Huffman Tree → Canonical Codes 
→ Bit Stream → Byte Alignment → Archive with Header
```

---

## Platform Support

| Feature | Windows | Linux |
|---------|---------|-------|
| Single file | Yes | Yes |
| Directory recursive | Yes | Yes |
| Wildcard `* ?` | Yes | Yes |
| Progress animation | Yes | Yes |
| Color output | Yes | Yes |

---

## Known Issues & Solutions

| Issue | Cause | Solution |
|-------|-------|----------|
| Console flashes after compression | Residual newline in input buffer | Fixed: consume all residual characters with `getchar()` loop |
| Recursive compression loses files | Directory entries added to file list | Fixed: separate directory recursion from file collection |
| Large file compression slow | Progress refresh every 1KB | Fixed: refresh by percentage (max 100 times) |
| Path separator mismatch | `\` vs `/` inconsistency | Fixed: unify to `/` internally |
| Wildcard decompression fails | `compressed_len` written as 1 byte | Fixed: write full 8-byte `ull` |

---

## License

MIT License - see LICENSE file for details.

---

## Author

Developed as a course project for Data Structures and Algorithms.

