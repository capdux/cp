# fresh — AI Agent Quick Ref

Zero-allocation RFC 9110 HTTP conditional request freshness testing (ETag, If-None-Match, If-Modified-Since, Cache-Control) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RequestHeaders,
    ResponseHeaders,
    TrimRange,
    ParsedEtag,
    trimRange,
    hasNoCache,
    parseEtagSlice,
    etagWeakMatch,
} from "fresh";
```

## Core Types & Constants

```cp
class RequestHeaders { if_none_match: &[]u8, if_modified_since: &[]u8, cache_control: &[]u8 }
class ResponseHeaders { etag: &[]u8, last_modified: &[]u8 }
class TrimRange { start: usize, end: usize }
class ParsedEtag { start: usize, end: usize, weak: bool, valid: bool }
```

## API Signatures

```cp
fn trimRange(s: &[]u8, start: usize, end: usize): TrimRange;
fn hasNoCache(cache_control: &[]u8): bool;
fn parseEtagSlice(s: &[]u8, raw_start: usize, raw_end: usize): ParsedEtag;
fn etagWeakMatch(a: &[]u8, b: &[]u8): bool;
fn etagStrongMatch(a: &[]u8, b: &[]u8): bool;
fn parse2Digits(s: &[]u8, pos: usize): i32;
fn parse4Digits(s: &[]u8, pos: usize): i32;
fn parseMonth(s: &[]u8, pos: usize): i32;
fn isValidWeekday(s: &[]u8, pos: usize): bool;
fn isLeapYear(y: u64): bool;
fn daysInMonth(month: i32, leap: bool): i32;
fn parseHttpDate(date_str: &[]u8): i64;
fn matchIfNoneMatch(inm: &[]u8, res_etag: &[]u8): bool;
fn isFresh(req: RequestHeaders, res: ResponseHeaders): bool;
```

## Minimal Usage

```cp
import { parseEtagSlice } from "fresh";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = parseEtagSlice(out_buf[0..256], 0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"fresh"`; install via `cap add fresh`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
