# postgres-interval — AI Agent Quick Ref

Zero-allocation PostgreSQL INTERVAL parser and ISO-8601 / PostgreSQL bidirectional formatter in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PgInterval,
    PgivWriter,
    PgivIntRes,
    PgivFracRes,
    pgivInvalid,
    pgivZero,
    pgivSkipSpace,
    pgivParseUint,
} from "postgres-interval";
```

## Core Types & Constants

```cp
class PgInterval { valid: bool, years: i32, months: i32, days: i32, /* +4 fields */ }
class PgivWriter { buf: []u8, pos: usize, overflow: bool } // methods: writeByte(...) +5
class PgivIntRes { ok: bool, val: i64, next: usize }
class PgivFracRes { ms: i32, next: usize }
```

## API Signatures

```cp
fn pgivInvalid(): PgInterval;
fn pgivZero(): PgInterval;
fn pgivSkipSpace(s: string, start: usize): usize;
fn pgivParseUint(s: string, start: usize): PgivIntRes;
fn pgivParseFracMs(s: string, start: usize): PgivFracRes;
fn pgivAbsI32ToU64(v: i32): u64;
fn pgivParseIso(s: string, start: usize, end: usize): PgInterval;
fn parsePgInterval(input: string): PgInterval;
fn toIsoDuration(iv: PgInterval, out: []u8): usize;
fn toPostgresString(iv: PgInterval, out: []u8): usize;
fn totalSecondsApprox(iv: PgInterval): i64;
```

## Minimal Usage

```cp
import { parsePgInterval } from "postgres-interval";

pub fn main() {
    let result = parsePgInterval("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"postgres-interval"`; install via `cap add postgres-interval`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
