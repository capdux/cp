# parse-svg-path — AI Agent Quick Ref

Zero-allocation SVG path `d` attribute parser, bounding box calculator, and canonical formatter for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PathCmd,
    ParsedPath,
    PathBounds,
    ParsedNum,
    BoundsAcc,
    skipWs,
    skipWsComma,
    expectedArgCount,
} from "parse-svg-path";
```

## Core Types & Constants

```cp
class PathCmd { code: u8, relative: bool, args: [7]f64, arg_count: u8 } // methods: empty(...)
class ParsedPath { cmds: [64]PathCmd, count: usize, valid: bool } // methods: invalid(...) +1
class PathBounds { valid: bool, min_x: f64, min_y: f64, max_x: f64, /* +3 fields */ } // methods: empty(...)
class ParsedNum { ok: bool, val: f64, next_pos: usize }
class BoundsAcc { has_point: bool, min_x: f64, min_y: f64, max_x: f64, /* +1 fields */ }
```

## API Signatures

```cp
fn skipWs(s: string, start: usize): usize;
fn skipWsComma(s: string, start: usize): usize;
fn expectedArgCount(code: u8): u8?;
fn isLowerCmd(code: u8): bool;
fn isNumberStart(c: u8): bool;
fn pow10(exp: i32): f64;
fn parseSvgNumber(s: string, start: usize): ParsedNum;
fn parseSvgFlag(s: string, start: usize): ParsedNum;
fn parseSvgPath(d: string): ParsedPath;
fn includePoint(acc: BoundsAcc, x: f64, y: f64): BoundsAcc;
fn computeBounds(p: ParsedPath): PathBounds;
fn appendByte(out: []u8, pos: usize, c: u8): usize;
fn appendFloatCompact(out: []u8, start_pos: usize, val: f64): usize;
fn formatSvgPath(p: ParsedPath, out: []u8): string;
```

## Minimal Usage

```cp
import { parseSvgNumber } from "parse-svg-path";

pub fn main() {
    let result = parseSvgNumber("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"parse-svg-path"`; install via `cap add parse-svg-path`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
