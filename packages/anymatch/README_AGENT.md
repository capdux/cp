# anymatch — AI Agent Quick Ref

Zero-heap file path & glob matcher with negation support, composing normalize-path and is-glob in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    startsWithDirPrefix,
    globMatchAt,
    matchPattern,
    anymatchIndex,
    anymatch,
    countGlobPatterns,
} from "anymatch";
```

## API Signatures

```cp
fn startsWithDirPrefix(path: &[]u8, prefix: &[]u8): bool;
fn globMatchAt(pat: &[]u8, pi_in: usize, txt: &[]u8, ti_in: usize): bool;
fn matchPattern(pattern: string, test_path: string): bool;
fn anymatchIndex(patterns: &[]string, test_path: string): i64;
fn anymatch(patterns: &[]string, test_path: string): bool;
fn countGlobPatterns(patterns: &[]string): usize;
```

## Minimal Usage

```cp
import { matchPattern } from "anymatch";

pub fn main() {
    let result = matchPattern("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"anymatch"`; install via `cap add anymatch`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add anymatch
cpc check src/main.cp
```
