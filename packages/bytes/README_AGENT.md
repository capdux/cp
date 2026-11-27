# bytes — AI Agent Quick Ref

Unified zero-heap byte size parser/formatter (IEC KiB/MiB/GiB & SI KB/MB/GB) and zero-copy byte buffer cursors (BytesReader/BytesMut) for CP (superset of npm bytes & crates.io bytes) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ReadU8Result,
    ReadU16Result,
    ReadU32Result,
    ReadU64Result,
    BytesReader,
    BytesMut,
    bytUnitMultiplier,
    bytU64Len,
} from "bytes";
```

## Core Types & Constants

```cp
class ReadU8Result { ok: bool, val: u8 }
class ReadU16Result { ok: bool, val: u16 }
class ReadU32Result { ok: bool, val: u32 }
class ReadU64Result { ok: bool, val: u64 }
class BytesReader { data: &[]u8, pos: usize } // methods: new(...) +12
class BytesMut { buf: [512]u8, used: usize } // methods: new(...) +17
```

## API Signatures

```cp
fn bytUnitMultiplier(unit: string, binary_default: bool): u64;
fn bytU64Len(val: u64): usize;
fn parseBytes(input: string, binary_default: bool): i64;
fn parse(input: string): i64;
fn formatBytes(bytes: u64, binary: bool, decimals: u32, out: []u8): usize;
fn formatIec(bytes: u64, out: []u8): usize;
fn formatSi(bytes: u64, out: []u8): usize;
fn bytbCopyBytes(dst: []u8, src: &[]u8, n: usize);
```

## Minimal Usage

```cp
import { parseBytes } from "bytes";

pub fn main() {
    let result = parseBytes("example", false);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"bytes"`; install via `cap add bytes`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
