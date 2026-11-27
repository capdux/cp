# bytecount — AI Agent Quick Ref

Zero-heap fast byte frequency counting, SWAR 4-byte parallel counter, and UTF-8 char/line counters (crates.io bytecount equivalent, reuses memchr) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    bcSumVec,
    count,
    countFast,
    numChars,
    countLines,
    countAsciiWhitespace,
    countInRange,
} from "bytecount";
```

## API Signatures

```cp
fn bcSumVec(acc: BcVec16): usize;
fn count(haystack: &[]u8, needle: u8): usize;
fn countFast(haystack: &[]u8, needle: u8): usize;
fn numChars(utf8_bytes: &[]u8): usize;
fn countLines(buf: &[]u8): usize;
fn countAsciiWhitespace(buf: &[]u8): usize;
fn countInRange(buf: &[]u8, lo: u8, hi: u8): usize;
```

## Minimal Usage

```cp
import { bcSumVec } from "bytecount";

pub fn main() {
    let result = bcSumVec(/* acc: BcVec16 */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"bytecount"`; install via `cap add bytecount`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add bytecount
cpc check src/main.cp
```
