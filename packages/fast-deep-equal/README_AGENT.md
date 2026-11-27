# fast-deep-equal — AI Agent Quick Ref

Zero-heap semantic JSON deep equality checker and root type classifier in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ValueSpan,
    DecChar,
    isWs,
    skipWs,
    matchLit,
    hexDigit,
    invalidSpan,
    scanString,
} from "fast-deep-equal";
```

## Core Types & Constants

```cp
class ValueSpan { start: usize, end: usize, kind: u8, valid: bool }
class DecChar { code: u32, next_pos: usize, done: bool }
```

## API Signatures

```cp
fn isWs(ch: u8): bool;
fn skipWs(s: string, pos: usize): usize;
fn matchLit(s: string, pos: usize, lit: string): bool;
fn hexDigit(ch: u8): i32;
fn invalidSpan(): ValueSpan;
fn scanString(s: string, pos: usize): ValueSpan;
fn scanNumber(s: string, pos: usize): ValueSpan;
fn parseValueSpan(s: string, pos: usize, depth: u32): ValueSpan;
fn kindOfJson(s: string): u8;
fn isPlainObjectJson(s: string): bool;
fn parseJsonNumberF64(s: string): f64;
fn nextDecodedChar(s: string, pos: usize): DecChar;
fn jsonStringsEqual(a: string, b: string): bool;
fn countObjectMembers(obj: string): usize;
fn arraysDeepEqual(a: string, b: string, depth: u32): bool;
fn objectsDeepEqual(a: string, b: string, depth: u32): bool;
fn valuesDeepEqual(a: string, b: string, kind: u8, depth: u32): bool;
fn jsonDeepEqual(a: string, b: string): bool;
```

## Minimal Usage

```cp
import { parseValueSpan } from "fast-deep-equal";

pub fn main() {
    let result = parseValueSpan("example", 0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"fast-deep-equal"`; install via `cap add fast-deep-equal`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
