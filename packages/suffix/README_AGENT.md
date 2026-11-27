# suffix — AI Agent Quick Ref

Zero-heap static Suffix Array + Kasai O(N) LCP array with O(M log N) binary search, occurrence counting, and longest repeated substring detection. (`v1.0.0`, `MIT`)

## Import

```cp
import { LrsResult, SuffixArray, compareSuffixes } from "suffix";
```

## Core Types & Constants

```cp
class LrsResult { start: usize, len: usize, found: bool }
class SuffixArray { text: [256]u8, sa: [256]u16, lcp: [256]u16, text_len: usize } // methods: build(...) +9
```

## API Signatures

```cp
fn compareSuffixes(buf: &[]u8, n: usize, a: usize, b: usize): i32;
```

## Minimal Usage

```cp
import { compareSuffixes } from "suffix";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = compareSuffixes(out_buf[0..256], 0, 0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"suffix"`; install via `cap add suffix`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
