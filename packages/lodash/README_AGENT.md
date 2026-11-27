# lodash — AI Agent Quick Ref

Zero-allocation dot-path parser and deep JSON path reader (dot-prop / lodash.get / dlv parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PathSegment,
    ParsedDotPath,
    emptySegment,
    invalidPath,
    parseUsize,
    parseDotPath,
    skipSpaces,
    skipJsonString,
} from "lodash";
```

## Core Types & Constants

```cp
class PathSegment { is_index: bool, key: string, index: usize }
class ParsedDotPath { segs: [MAX_SEGMENTS]PathSegment, count: usize, valid: bool } // methods: get(...)
```

## API Signatures

```cp
fn emptySegment(): PathSegment;
fn invalidPath(): ParsedDotPath;
fn parseUsize(s: string): usize?;
fn parseDotPath(path: string, key_buf: []u8): ParsedDotPath;
fn skipSpaces(s: string, pos: usize): usize;
fn skipJsonString(s: string, start: usize): usize?;
fn skipJsonValue(s: string, start: usize): usize?;
fn jsonStringEquals(rawQuoted: string, target: string): bool;
fn lookupObjectKey(obj: string, key: string): string?;
fn lookupArrayIndex(arr: string, targetIdx: usize): string?;
fn getJsonPath(json: string, path: string): string?;
fn getJsonString(json: string, path: string): string?;
fn hasJsonPath(json: string, path: string): bool;
fn escapePathSegment(seg: string, out: []u8): string;
```

## Minimal Usage

```cp
import { parseUsize } from "lodash";

pub fn main() {
    let result = parseUsize("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"lodash"`; install via `cap add lodash`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
