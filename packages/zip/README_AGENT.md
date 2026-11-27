# zip — AI Agent Quick Ref

Zero-heap PKZIP Stored archive writer, local header & EOCD parser, and CRC-32 verifier (crates.io zip equivalent) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ZipLocalHeader,
    ZipEocd,
    zipEmptyLocalHeader,
    zipEmptyEocd,
    zipCopyStr,
    zipCopyBytes,
    writeStoredZipSingle,
    parseLocalHeader,
} from "zip";
```

## Core Types & Constants

```cp
class ZipLocalHeader { version_needed: u16, flags: u16, method: u16, crc32: u32, /* +6 fields */ }
class ZipEocd { disk_num: u16, total_entries: u16, cd_size: u32, cd_offset: u32, /* +1 fields */ }
```

## API Signatures

```cp
fn zipEmptyLocalHeader(): ZipLocalHeader;
fn zipEmptyEocd(): ZipEocd;
fn zipCopyStr(dst: []u8, offset: usize, s: string);
fn zipCopyBytes(dst: []u8, offset: usize, src: []u8);
fn writeStoredZipSingle(filename: string, payload: []u8, out: []u8): usize?;
fn parseLocalHeader(buf: []u8, offset: usize): ZipLocalHeader;
fn findAndParseEocd(buf: []u8): ZipEocd;
fn verifyStoredEntry(buf: []u8, hdr: ZipLocalHeader): bool;
```

## Minimal Usage

```cp
import { writeStoredZipSingle } from "zip";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = writeStoredZipSingle("example", out_buf[0..256], buf_1[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"zip"`; install via `cap add zip`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
