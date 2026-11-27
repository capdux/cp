# querystring — AI Agent Quick Ref

Zero-allocation URL query string parser, stringifier & parameter extractor (Node.js querystring & npm query-string v9 parity) composing percent-encoding for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    QueryEntry,
    qstFindByte,
    qstNeedsDecode,
    qstHasSlashBefore,
    escapeQuery,
    unescapeQuery,
    extractUrlQuery,
    parseQueryString,
} from "querystring";
```

## Core Types & Constants

```cp
class QueryEntry { key: string, value: string }
```

## API Signatures

```cp
fn qstFindByte(s: string, start: usize, end: usize, target: u8): usize;
fn qstNeedsDecode(s: string): bool;
fn qstHasSlashBefore(s: string, end: usize): bool;
fn escapeQuery(raw: string, out: []u8): usize;
fn unescapeQuery(raw: string, out: []u8): usize;
fn extractUrlQuery(url: string): string;
fn parseQueryString(input: string, sep: u8, eq: u8, out_entries: []QueryEntry): usize;
fn stringifyQueryString(entries: &[]QueryEntry, sep: u8, eq: u8, out: []u8): usize;
fn pickQueryParam(url_or_query: string, target_key: string, out: []u8): usize?;
```

## Minimal Usage

```cp
import { parseQueryString } from "querystring";

pub fn main() {
    let result = parseQueryString("example", 0, 0, /* out_entries: []QueryEntry */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"querystring"`; install via `cap add querystring`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
