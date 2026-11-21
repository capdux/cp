# `cap:bzip2` — bzip2 `.bz2` Container & MSB-First Bitstream Zero-Allocation Parser

Pure CP, zero-allocation parser for bzip2 `.bz2` streams: 4-byte stream header (`"BZh1"`..`"BZh9"`), MSB-first bitstream reader, 48-bit π block header (with L1/L2 symbol bitmap decoding), 48-bit √π End-Of-Stream footer, and bzip2 MSB-first CRC32.

## Protocol Constants

- `BZIP2_MAGIC = "BZh"`, `BZIP2_HEADER_SIZE = 4`
- `BZIP2_BLOCK_MAGIC = 0x314159265359` (48-bit BCD of π)
- `BZIP2_EOS_MAGIC = 0x177245385090` (48-bit BCD of √π)
- `BZIP2_MIN_LEVEL = 1`, `BZIP2_MAX_LEVEL = 9`

## Core Types & Functions

| Symbol | Description |
| :--- | :--- |
| `bzip2Crc32(data: []const u8): u32` | MSB-first CRC32 (forward polynomial `0x04C11DB7`, init `0xFFFFFFFF`, final XOR; `"123456789" -> 0xFC891918`). |
| `bzip2CombineCrc(combined: u32, block_crc: u32): u32` | Rotates `combined` left by 1 bit and XORs with `block_crc`. |
| `Bzip2BitReader` | Zero-copy MSB-first bitstream reader (`init`, `readBit`, `readBits(1..32)`, `readU48`, `alignToByte`, `bitsRemaining`). |
| `isBzip2(data: []const u8): bool` | Fast `"BZh"` + `'1'..'9'` check. |
| `parseBzip2Header(data: []const u8): Bzip2StreamHeader?` | Parses stream header (`blockSizeBytes() = level * 100000`). |
| `parseBzip2BlockHeader(br: *Bzip2BitReader): Bzip2BlockHeader?` | Parses 48-bit π magic, 32-bit CRC, randomized bit, 24-bit `orig_ptr`, L1/L2 symbol bitmap count, `num_trees (2..6)`, `num_selectors (1..18002)`. |
| `parseBzip2Eos(br: *Bzip2BitReader): Bzip2EosFooter?` | Parses 48-bit √π EOS magic and 32-bit combined CRC. |
| `probeBzip2Stream(data: []const u8): Bzip2StreamProbe?` | Probes empty `.bz2` streams or first compressed block header. |

## Example

```cp
import { probeBzip2Stream, bzip2Crc32 } from "cap:bzip2";

fn inspectBz2(data: []const u8) {
    if (p := probeBzip2Stream(data)) {
        * = p.level;
        * = p.is_empty_stream;
        * = p.first_block_crc;
    }
    * = bzip2Crc32("123456789");
}
```
