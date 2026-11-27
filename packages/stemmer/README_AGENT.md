# stemmer — AI Agent Quick Ref

Zero-allocation Martin Porter (1980) English Stemmer (Steps 1a-5b) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    isConsonant,
    measurePrefix,
    measure,
    containsVowel,
    endsDoubleConsonant,
    endsCvc,
    tryStep2,
    tryStep3,
} from "stemmer";
```

## API Signatures

```cp
fn isConsonant(buf: &[]u8, idx: usize): bool;
fn measurePrefix(buf: &[]u8, end: usize): usize;
fn measure(word: &[]u8): usize;
fn containsVowel(buf: &[]u8, end: usize): bool;
fn endsDoubleConsonant(buf: &[]u8, end: usize): bool;
fn endsCvc(buf: &[]u8, end: usize): bool;
fn tryStep2(buf: []u8, len: usize): usize;
fn tryStep3(buf: []u8, len: usize): usize;
fn tryStep4(buf: []u8, len: usize): usize;
fn stem(word: string, out: []u8): usize;
fn sameStem(a: string, b: string): bool;
```

## Minimal Usage

```cp
import { stem } from "stemmer";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = stem("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"stemmer"`; install via `cap add stemmer`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
