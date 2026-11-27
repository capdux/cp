# braces — AI Agent Quick Ref

Zero-allocation Bash/Glob brace expansion and sequence range generator (braces + fill-range parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    BraceResult,
    BrExpander,
    BrInt,
    BrRangeSpec,
    BrSpan,
    brInitExpander,
    brToResult,
    brPushItem,
} from "braces";
```

## Core Types & Constants

```cp
class BraceResult { buf: []u8, starts: [BR_MAX_ITEMS]usize, lens: [BR_MAX_ITEMS]usize, count: usize, /* +1 fields */ } // methods: itemAt(...)
class BrExpander { out: []u8, pos: usize, starts: [BR_MAX_ITEMS]usize, lens: [BR_MAX_ITEMS]usize, /* +2 fields */ }
class BrInt { ok: bool, val: i64 }
class BrRangeSpec { ok: bool, startStr: string, endStr: string, step: i32 }
class BrSpan { found: bool, openIdx: usize, closeIdx: usize, isCommaSet: bool }
```

## API Signatures

```cp
fn brInitExpander(out: []u8): BrExpander;
fn brToResult(e: &mut BrExpander): BraceResult;
fn brPushItem(e: &mut BrExpander, s: string);
fn brIsDigit(c: u8): bool;
fn brIsUpper(c: u8): bool;
fn brIsLower(c: u8): bool;
fn brParseInt(s: string): BrInt;
fn brFormatSimpleInt(v: i64, buf: &mut [32]u8): string;
fn brParseRangeSpec(body: string): BrRangeSpec;
fn brHasTopLevelComma(body: string): bool;
fn brFindExpandableBrace(pat: string): BrSpan;
fn brBuildCombined(prefix: string, mid: string, suffix: string, buf: &mut [BR_MAX_WORK_BUF]u8): usize;
fn brEmitToken(e: &mut BrExpander, prefix: string, tok: string, suffix: string, depth: usize);
fn brExpandRangeWithContext(e: &mut BrExpander, prefix: string, startStr: string, endStr: string, stepIn: i32, suffix: string, depth: usize);
fn brExpandRec(e: &mut BrExpander, pat: string, depth: usize);
fn expandBraces(pattern: string, out: []u8): BraceResult;
fn fillRange(start: string, end: string, step: i32, out: []u8): BraceResult;
```

## Minimal Usage

```cp
import { brParseInt } from "braces";

pub fn main() {
    let result = brParseInt("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"braces"`; install via `cap add braces`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
