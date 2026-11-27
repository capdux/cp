# winnow — AI Agent Quick Ref

Zero-allocation byte-level zero-copy parser combinators (winnow / nom parity) with rollback-on-failure cursor semantics for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Cursor,
    SpanResult,
    IntResult,
    UintResult,
    nbFailSpan,
    tag,
    tagNoCase,
    charLit,
} from "winnow";
```

## Core Types & Constants

```cp
class Cursor { data: string, pos: usize } // methods: new(...) +5
class SpanResult { ok: bool, start: usize, len: usize }
class IntResult { ok: bool, value: i64 }
class UintResult { ok: bool, value: u64 }
```

## API Signatures

```cp
fn nbFailSpan(start: usize): SpanResult;
fn tag(cur: &mut Cursor, lit: string): bool;
fn tagNoCase(cur: &mut Cursor, lit: string): bool;
fn charLit(cur: &mut Cursor, ch: u8): bool;
fn take(cur: &mut Cursor, count: usize): SpanResult;
fn takeUntil(cur: &mut Cursor, needle: string): SpanResult;
fn takeTillByte(cur: &mut Cursor, stop: u8): SpanResult;
fn space0(cur: &mut Cursor): SpanResult;
fn multispace0(cur: &mut Cursor): SpanResult;
fn digit1(cur: &mut Cursor): SpanResult;
fn alpha1(cur: &mut Cursor): SpanResult;
fn alphanumeric1(cur: &mut Cursor): SpanResult;
fn hexDigit1(cur: &mut Cursor): SpanResult;
fn parseU64(cur: &mut Cursor): UintResult;
fn parseI64(cur: &mut Cursor): IntResult;
fn delimited(cur: &mut Cursor, open: u8, close: u8): SpanResult;
```

## Minimal Usage

```cp
import { tag } from "winnow";

pub fn main() {
    let result = tag(/* cur: &mut Cursor */, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"winnow"`; install via `cap add winnow`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
