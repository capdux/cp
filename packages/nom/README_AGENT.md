# nom — AI Agent Quick Ref

Zero-allocation streaming parser combinator primitives (tag, tagNoCase, take, takeUntil, character classes, delimited/separated sequences, integer parsers) for CP (clean-room MIT, inspired by Rust nom v7). (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ParseRes,
    IntRes,
    UintRes,
    nomFail,
    nomOk,
    nomContainsByte,
    nomIsDigit,
    nomIsHexDigit,
} from "nom";
```

## Core Types & Constants

```cp
class ParseRes { ok: bool, rest: string, matched: string }
class IntRes { ok: bool, rest: string, value: i64 }
class UintRes { ok: bool, rest: string, value: u64 }
```

## API Signatures

```cp
fn nomFail(input: string): ParseRes;
fn nomOk(input: string, split_at: usize): ParseRes;
fn nomContainsByte(set: string, b: u8): bool;
fn nomIsDigit(b: u8): bool;
fn nomIsHexDigit(b: u8): bool;
fn nomHexVal(b: u8): u64;
fn nomIsAlpha(b: u8): bool;
fn nomIsAlnum(b: u8): bool;
fn nomIsSpace(b: u8): bool;
fn nomIsMultispace(b: u8): bool;
fn nomSliceEq(a: string, b: string): bool;
fn tag(input: string, expected: string): ParseRes;
fn tagNoCase(input: string, expected: string): ParseRes;
fn take(input: string, count: usize): ParseRes;
fn takeUntil(input: string, needle: string): ParseRes;
fn isNot(input: string, forbidden_chars: string): ParseRes;
fn isA(input: string, allowed_chars: string): ParseRes;
fn digit0(input: string): ParseRes;
fn digit1(input: string): ParseRes;
fn hexDigit1(input: string): ParseRes;
fn alpha0(input: string): ParseRes;
fn alpha1(input: string): ParseRes;
fn alphanumeric0(input: string): ParseRes;
fn alphanumeric1(input: string): ParseRes;
fn space0(input: string): ParseRes;
fn space1(input: string): ParseRes;
fn multispace0(input: string): ParseRes;
fn multispace1(input: string): ParseRes;
fn lineEnding(input: string): ParseRes;
fn notLineEnding(input: string): ParseRes;
fn delimitedTag(input: string, open: string, close: string): ParseRes;
fn precededTag(input: string, prefix: string): ParseRes;
fn terminatedTag(input: string, suffix: string): ParseRes;
fn separatedPairTag(input: string, sep: string): ParseRes;
fn parseU64(input: string): UintRes;
fn parseI64(input: string): IntRes;
fn parseHexU64(input: string): UintRes;
```

## Minimal Usage

```cp
import { isNot } from "nom";

pub fn main() {
    let result = isNot("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"nom"`; install via `cap add nom`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
