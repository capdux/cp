# pino — AI Agent Quick Ref

Zero-heap structured NDJSON logger with numeric levels (10..60), safe JSON escaping, and child fields (clean-room port of npm pino) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PinoField,
    PinoLogger,
    pinoPush,
    pinoPushJsonStr,
    pinoPushU64,
    pinoPushI64,
    levelLabel,
} from "pino";
```

## Core Types & Constants

```cp
class PinoField { key: string, kind: u8, str_val: string, int_val: i64, /* +1 fields */ }
class PinoLogger { min_level: u8, name: string, pid: u32, hostname: string, /* +2 fields */ } // methods: new(...) +5
```

## API Signatures

```cp
fn pinoPush(out: []u8, pos: usize, s: string): usize;
fn pinoPushJsonStr(out: []u8, pos: usize, s: string): usize;
fn pinoPushU64(out: []u8, pos: usize, v: u64): usize;
fn pinoPushI64(out: []u8, pos: usize, v: i64): usize;
fn levelLabel(level: u8): string;
```

## Minimal Usage

```cp
import { pinoPush } from "pino";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = pinoPush(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"pino"`; install via `cap add pino`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
