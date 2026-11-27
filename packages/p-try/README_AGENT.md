# p-try — AI Agent Quick Ref

Zero-heap structured sync/async execution outcome wrapper preventing escaping errors, with safe integer parsing, division, and short-circuit chaining (clean-room port of npm p-try) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    TryOutcome,
    ptryIsSpace,
    ptryIsDigit,
    ptryI64Min,
    ptryAbsU64,
    tryOkInt,
    tryOkStr,
    tryErr,
} from "p-try";
```

## Core Types & Constants

```cp
class TryOutcome { ok: bool, value_i64: i64, value_str: string, err_code: i32, /* +1 fields */ } // methods: okInt(...) +4
```

## API Signatures

```cp
fn ptryIsSpace(ch: u8): bool;
fn ptryIsDigit(ch: u8): bool;
fn ptryI64Min(): i64;
fn ptryAbsU64(v: i64): u64;
fn tryOkInt(v: i64): TryOutcome;
fn tryOkStr(s: string): TryOutcome;
fn tryErr(code: i32, msg: string): TryOutcome;
fn unwrapOrInt(o: TryOutcome, fallback: i64): i64;
fn unwrapOrStr(o: TryOutcome, fallback: string): string;
fn tryParseI64(s: string): TryOutcome;
fn tryDivI64(a: i64, b: i64): TryOutcome;
fn tryChain(first: TryOutcome, next_ok: bool, next_val: i64, next_err_code: i32, next_err_msg: string): TryOutcome;
```

## Minimal Usage

```cp
import { tryOkStr } from "p-try";

pub fn main() {
    let result = tryOkStr("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"p-try"`; install via `cap add p-try`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
