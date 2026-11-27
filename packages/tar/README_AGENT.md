# tar — AI Agent Quick Ref

Zero-heap POSIX USTAR 512-byte tar archive header codec and streaming cursor (crates.io tar equivalent) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    TarHeader,
    TarEntrySpan,
    TarArchiveCursor,
    tarEmptyHeader,
    tarWriteOctal,
    tarParseOctal,
    tarComputeChecksum,
    tarIsUstarMagic,
} from "tar";
```

## Core Types & Constants

```cp
class TarHeader { name: string, mode: u32, uid: u32, gid: u32, /* +6 fields */ }
class TarEntrySpan { header: TarHeader, data_offset: usize, data_len: usize }
class TarArchiveCursor { offset: usize, finished: bool } // methods: init(...) +2
```

## API Signatures

```cp
fn tarEmptyHeader(): TarHeader;
fn tarWriteOctal(out: []u8, offset: usize, digits: usize, val: u64): bool;
fn tarParseOctal(buf: []u8, offset: usize, width: usize, require_digit: bool): u64?;
fn tarComputeChecksum(block512: []u8): u32;
fn tarIsUstarMagic(block512: []u8): bool;
fn isZeroBlock(block512: []u8): bool;
fn tarPaddedSize(raw_size: u64): u64;
fn encodeTarHeader(name: string, size: u64, mode: u32, mtime: u64, typeflag: u8, out512: []u8): bool;
fn parseTarHeader(block512: []u8): TarHeader;
```

## Minimal Usage

```cp
import { encodeTarHeader } from "tar";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeTarHeader("example", 0, 0, 0, 0, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"tar"`; install via `cap add tar`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
