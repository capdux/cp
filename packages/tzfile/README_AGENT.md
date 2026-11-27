# tzfile — AI Agent Quick Ref

Zero-heap RFC 8536 TZif binary header parser, POSIX TZ string parser, and UTC offset transition evaluator (crates.io tzfile equivalent) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    TzifHeader,
    PosixTz,
    TzTransition,
    TzAbbrRes,
    TzOffRes,
    tzReadU32Be,
    isTzifMagic,
    parseTzifHeader,
} from "tzfile";
```

## Core Types & Constants

```cp
class TzifHeader { version: u8, isutcnt: u32, isstdcnt: u32, leapcnt: u32, /* +3 fields */ }
class PosixTz { std_abbr: string, std_offset_secs: i32, has_dst: bool, dst_abbr: string, /* +1 fields */ }
class TzTransition { at_unix: i64, offset_secs: i32, is_dst: bool }
class TzAbbrRes { abbr: string, next_pos: usize, ok: bool }
class TzOffRes { secs: i32, next_pos: usize, ok: bool }
```

## API Signatures

```cp
fn tzReadU32Be(buf: []u8, off: usize): u32;
fn isTzifMagic(buf: []u8): bool;
fn parseTzifHeader(buf: []u8): TzifHeader?;
fn tzIsAlpha(c: u8): bool;
fn tzIsDigit(c: u8): bool;
fn tzParseAbbr(s: string, pos_in: usize): TzAbbrRes;
fn tzParsePosixOffset(s: string, pos_in: usize): TzOffRes;
fn parsePosixTz(spec: string): PosixTz?;
fn lookupTransitionOffset(transitions: []TzTransition, unix_secs: i64, default_offset: i32): i32;
fn utcToLocalSeconds(unix_secs: i64, offset_secs: i32): i64;
```

## Minimal Usage

```cp
import { parsePosixTz } from "tzfile";

pub fn main() {
    let result = parsePosixTz("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"tzfile"`; install via `cap add tzfile`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
