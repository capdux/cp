# metaphone — AI Agent Quick Ref

Zero-allocation Lawrence Philips Metaphone English phonetic encoding algorithm in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import { isVowel, metaphone, soundsLike } from "metaphone";
```

## API Signatures

```cp
fn isVowel(b: u8): bool;
fn metaphone(word: string, max_len: usize, out: []u8): usize;
fn soundsLike(a: string, b: string): bool;
```

## Minimal Usage

```cp
import { metaphone } from "metaphone";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = metaphone("example", 0, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"metaphone"`; install via `cap add metaphone`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add metaphone
cpc check src/main.cp
```
