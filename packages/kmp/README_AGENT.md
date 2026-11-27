# kmp — AI Agent Quick Ref

Zero-allocation O(N+M) Knuth-Morris-Pratt (KMP) substring search and LPS failure function engine for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    computeLps,
    findFirst,
    findAll,
    countMatches,
    contains,
} from "kmp";
```

## API Signatures

```cp
fn computeLps(pattern: &[]u8, out_lps: []usize): usize;
fn findFirst(haystack: &[]u8, needle: &[]u8): i64;
fn findAll(haystack: &[]u8, needle: &[]u8, overlapping: bool, out_offsets: []usize): usize;
fn countMatches(haystack: &[]u8, needle: &[]u8, overlapping: bool): usize;
fn contains(haystack: &[]u8, needle: &[]u8): bool;
```

## Minimal Usage

```cp
import { computeLps } from "kmp";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = computeLps(out_buf[0..256], /* out_lps: []usize */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"kmp"`; install via `cap add kmp`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add kmp
cpc check src/main.cp
```
