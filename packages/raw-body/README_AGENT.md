# raw-body — AI Agent Quick Ref

Zero-allocation HTTP request body byte-limit enforcer, Content-Length verifier, streaming chunk accumulator, and RFC 3629 UTF-8 validator for CP (clean-room parity with npm raw-body) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RawBodyOptions,
    RawBodyResult,
    RawBodyReader,
    rwbIsCont,
    rwbUnitMultiplier,
    isValidUtf8,
    parseByteLimit,
} from "raw-body";
```

## Core Types & Constants

```cp
class RawBodyOptions { limit: usize, expected_len: usize, has_expected_len: bool, validate_utf8: bool } // methods: defaultOpts(...) +1
class RawBodyResult { status: u16, bytes_written: usize } // methods: isOk(...)
class RawBodyReader { opts: RawBodyOptions, received: usize, failed_status: u16, finished: bool } // methods: new(...) +5
```

## API Signatures

```cp
fn rwbIsCont(b: u8): bool;
fn rwbUnitMultiplier(unit: string): u64;
fn isValidUtf8(data: &[]u8): bool;
fn parseByteLimit(spec: string): usize?;
```

## Minimal Usage

```cp
import { parseByteLimit } from "raw-body";

pub fn main() {
    let result = parseByteLimit("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"raw-body"`; install via `cap add raw-body`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
