# @paralleldrive/cuid2 — AI Agent Quick Ref

Zero-heap collision-resistant, URL-safe, lowercase CUID2 ID generator and validator (clean-room port of npm @paralleldrive/cuid2) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Cuid2Generator,
    c2Mix,
    c2Base36Char,
    c2LetterChar,
    isCuid2,
    isCuid2WithBounds,
} from "@paralleldrive/cuid2";
```

## Core Types & Constants

```cp
class Cuid2Generator { state: u64, counter: u64, fingerprint_hash: u64, id_len: usize } // methods: new(...) +2
```

## API Signatures

```cp
fn c2Mix(x: u64): u64;
fn c2Base36Char(v: u64): u8;
fn c2LetterChar(v: u64): u8;
fn isCuid2(id: string): bool;
fn isCuid2WithBounds(id: string, min_len: usize, max_len: usize): bool;
```

## Minimal Usage

```cp
import { isCuid2 } from "@paralleldrive/cuid2";

pub fn main() {
    let result = isCuid2("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@paralleldrive/cuid2"`; install via `cap add @paralleldrive/cuid2`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
