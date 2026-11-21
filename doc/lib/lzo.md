# `cap:lzo` — `lzop` `.lzo` Container & Zero-Allocation LZO1X Decompressor

Pure CP, zero-allocation parser for the `lzop` `.lzo` container format (Adler32 & IEEE CRC32 verification, multi-block zero-copy iteration) paired with a complete, bounds-checked LZO1X decompressor.

## Protocol Constants

- Magic: `LZOP_MAGIC = [0x89, 'L', 'Z', 'O', 0x00, 0x0D, 0x0A, 0x1A, 0x0A]` (9 bytes)
- Methods: `METHOD_LZO1X_1 (1)`, `METHOD_LZO1X_1_15 (2)`, `METHOD_LZO1X_999 (3)`
- Flags: `F_ADLER32_D (0x01)`, `F_ADLER32_C (0x02)`, `F_STDIN (0x04)`, `F_STDOUT (0x08)`, `F_EXTRA_FIELD (0x40)`, `F_CRC32_D (0x100)`, `F_CRC32_C (0x200)`, `F_H_FILTER (0x800)`, `F_H_CRC32 (0x1000)`

## Core Types & Functions

| Symbol | Description |
| :--- | :--- |
| `lzoAdler32(data: []const u8): u32` | RFC 1950 Adler-32 checksum. |
| `lzoCrc32(data: []const u8): u32` | IEEE 802.3 CRC32 checksum. |
| `isLzop(data: []const u8): bool` | Fast 9-byte `lzop` magic check. |
| `parseLzopHeader(data: []const u8): LzopHeader?` | Parses `lzop` header and verifies `[9..cksum_pos]` with CRC32 (`F_H_CRC32`) or Adler32. |
| `iterLzopBlocks(header: LzopHeader, data: []const u8): LzopBlockIter` | Iterates blocks until the `0x00000000` EOF sentinel, verifying optional compressed Adler32/CRC32. |
| `lzo1xDecompress(src: []const u8, dst: *[]u8): usize?` | Safe zero-alloc LZO1X decompressor (literals, M1/M2/M3/M4 back-references, overlapping RLE, `0x11 0x00 0x00` EOS). |

## Example

```cp
import { parseLzopHeader, iterLzopBlocks, lzo1xDecompress } from "cap:lzo";

fn decodeStream(lzo_bytes: []const u8, out_buf: *[]u8) {
    if (hdr := parseLzopHeader(lzo_bytes)) {
        let it = iterLzopBlocks(hdr, lzo_bytes);
        while (blk := it.next()) {
            if (!blk.isUncompressed()) {
                * = lzo1xDecompress(blk.payload, out_buf);
            }
        }
    }
}
```
