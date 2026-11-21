# `cap:xz` — XZ `.xz` Container Format v1.1.0 Zero-Allocation Parser

Pure CP, zero-allocation parser for the XZ v1.1.0 container format: 12-byte Stream Header & Footer with IEEE 802.3 CRC32 verification, 7-bit Variable-Length Integer (VLI) codec, Block Header & Filter Chain (including LZMA2 dictionary size decoding), and Index Record zero-copy iteration.

## Protocol Constants

- Magic & Sizes:
  - `XZ_HEADER_MAGIC = [0xFD, '7', 'z', 'X', 'Z', 0x00]`, `XZ_FOOTER_MAGIC = ['Y', 'Z']`
  - `XZ_STREAM_HEADER_SIZE = 12`, `XZ_STREAM_FOOTER_SIZE = 12`
- Check Types:
  - `CHECK_NONE (0x00)`, `CHECK_CRC32 (0x01)`, `CHECK_CRC64 (0x04)`, `CHECK_SHA256 (0x0A)`
- Filter IDs:
  - `FILTER_DELTA (0x03)`, `FILTER_X86 (0x04)`, `FILTER_POWERPC (0x05)`, `FILTER_IA64 (0x06)`, `FILTER_ARM (0x07)`, `FILTER_ARMTHUMB (0x08)`, `FILTER_SPARC (0x09)`, `FILTER_ARM64 (0x0A)`, `FILTER_RISCV (0x0B)`, `FILTER_LZMA2 (0x21)`

## Core Types & Functions

| Symbol | Description |
| :--- | :--- |
| `xzCrc32(data: []const u8): u32` | IEEE 802.3 CRC32 (`0xEDB88320`, final XOR). |
| `decodeXzVli(data: []const u8): XzVliResult?` | Decodes 1..9 byte 7-bit little-endian VLI (max 63-bit). |
| `encodeXzVli(val: u64, out: *[]u8): usize?` | Encodes 63-bit integer into 1..9 VLI bytes. |
| `isXz(data: []const u8): bool` | Fast length (`>= 12`) and 6-byte magic check. |
| `parseXzHeader(data: []const u8): XzStreamHeader?` | Validates magic, stream flags, and CRC32 (`checkSize()`, `checkName()`). |
| `parseXzFooter(data: []const u8): XzStreamFooter?` | Validates `"YZ"`, flags, CRC32, and computes `backward_size = (raw + 1) * 4`. |
| `parseXzBlockHeader(data: []const u8): XzBlockHeader?` | Validates size byte, CRC32, optional sizes, 1..4 filters (`XzFilter.lzma2DictSize()`), and zero padding. |
| `parseXzIndex(data: []const u8): XzIndex?` | Validates `0x00` indicator, VLI records, 4-byte zero padding, and CRC32 (`records()` returns `XzIndexRecordIter`). |

## Example

```cp
import {
    parseXzHeader,
    parseXzBlockHeader,
    parseXzIndex,
    parseXzFooter,
} from "cap:xz";

fn inspectXz(hdr_bytes: []const u8, blk_bytes: []const u8) {
    if (hdr := parseXzHeader(hdr_bytes)) {
        * = hdr.checkSize();
        * = hdr.checkName();
    }
    if (blk := parseXzBlockHeader(blk_bytes)) {
        * = blk.filters[0].lzma2DictSize();
    }
}
```
