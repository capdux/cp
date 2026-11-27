# clf-parser — AI Agent Quick Ref

Zero-allocation Apache/Nginx Common Log Format (CLF) and Combined Log Format parser and formatter in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ClfRecord,
    invalidClf,
    isClfSpace,
    isLineEnd,
    parseClf,
    pushSlice,
    pushU64,
    formatClf,
} from "clf-parser";
```

## Core Types & Constants

```cp
class ClfRecord { host_start: usize, host_len: usize, user_start: usize, user_len: usize, /* +16 fields */ }
```

## API Signatures

```cp
fn invalidClf(): ClfRecord;
fn isClfSpace(b: u8): bool;
fn isLineEnd(b: u8): bool;
fn parseClf(line: &[]u8): ClfRecord;
fn pushSlice(dst: []u8, pos: usize, src: &[]u8): i64;
fn pushU64(dst: []u8, pos: usize, val: u64): i64;
fn formatClf(host: &[]u8, user: &[]u8, time: &[]u8, method: &[]u8, path: &[]u8, proto: &[]u8, status: u16, bytes: i64, dst: []u8): i64;
```

## Minimal Usage

```cp
import { isClfSpace } from "clf-parser";

pub fn main() {
    let result = isClfSpace(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"clf-parser"`; install via `cap add clf-parser`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
