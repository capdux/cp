# stringprep — AI Agent Quick Ref

Zero-heap RFC 3454 / RFC 4013 SASLprep & Nameprep string preparation for SCRAM-SHA-256, PostgreSQL, and LDAP in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PrepResult,
    DecCp,
    decodeUtf8,
    encodeUtf8,
    isMappedToNothing,
    isNonAsciiSpace,
    isSaslProhibited,
    saslprep,
} from "stringprep";
```

## Core Types & Constants

```cp
class PrepResult { len: usize, ok: bool }
class DecCp { cp: u32, width: usize, valid: bool }
```

## API Signatures

```cp
fn decodeUtf8(s: string, i: usize): DecCp;
fn encodeUtf8(cp: u32, out: []u8, pos: usize): i64;
fn isMappedToNothing(cp: u32): bool;
fn isNonAsciiSpace(cp: u32): bool;
fn isSaslProhibited(cp: u32): bool;
fn saslprep(input: string, out: []u8): PrepResult;
fn nameprepAscii(input: string, out: []u8): PrepResult;
```

## Minimal Usage

```cp
import { decodeUtf8 } from "stringprep";

pub fn main() {
    let result = decodeUtf8("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"stringprep"`; install via `cap add stringprep`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
