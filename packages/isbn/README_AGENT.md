# isbn — AI Agent Quick Ref

Zero-allocation ISO 2108 ISBN-10, ISBN-13, ISSN, and EAN-13 validator and converter for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    isbnToUpper,
    isbnIsDigit,
    stripHyphens,
    isbnNormalizeExact,
    computeIsbn13CheckDigit,
    isValidIsbn10,
    isValidEan13,
    isValidIsbn13,
} from "isbn";
```

## API Signatures

```cp
fn isbnToUpper(c: u8): u8;
fn isbnIsDigit(c: u8): bool;
fn stripHyphens(input: string, out: []u8): usize;
fn isbnNormalizeExact(input: string, expected_len: usize, out: []u8): bool;
fn computeIsbn13CheckDigit(prefix12: string): u8;
fn isValidIsbn10(input: string): bool;
fn isValidEan13(input: string): bool;
fn isValidIsbn13(input: string): bool;
fn isValidIssn(input: string): bool;
fn isValidIsbn(input: string): bool;
fn isbn10To13(isbn10: string, out: []u8): usize;
fn isbn13To10(isbn13: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { isbnNormalizeExact } from "isbn";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isbnNormalizeExact("example", 0, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"isbn"`; install via `cap add isbn`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
