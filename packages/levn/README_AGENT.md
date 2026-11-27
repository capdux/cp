# levn — AI Agent Quick Ref

Zero-heap Light Value Notation (levn) scalar, list, and object parser (`v0.4.1`, `MIT`)

## Import

```cp
import {
    LevnKind,
    LevnValue,
    LevnPair,
    lvnMakeInvalid,
    lvnTryParseNumber,
    parseLevnScalar,
    lvnFindTopLevelChar,
    lvnUnquoteKey,
} from "levn";
```

## Core Types & Constants

```cp
enum LevnKind { Null, Bool, Int, Float, String, Invalid }
class LevnValue { kind: LevnKind, bool_val: bool, int_val: i64, float_val: f64, /* +1 fields */ }
class LevnPair { key: string, value: LevnValue }
```

## API Signatures

```cp
fn lvnMakeInvalid(): LevnValue;
fn lvnTryParseNumber(s: string): LevnValue;
fn parseLevnScalar(input: string): LevnValue;
fn lvnFindTopLevelChar(s: string, start: usize, target: u8): usize;
fn lvnUnquoteKey(s: string): string;
fn parseLevnList(input: string, out: []LevnValue): usize;
fn parseLevnObject(input: string, out: []LevnPair): usize;
```

## Minimal Usage

```cp
import { parseLevnScalar } from "levn";

pub fn main() {
    let result = parseLevnScalar("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"levn"`; install via `cap add levn`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
