# smol_str — AI Agent Quick Ref

Zero-heap pure-CP O(1)-clone small string optimization (24-byte inline SSO + read-only slice fallback, crates.io smol_str equivalent) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SmolStr,
    smolGetByte,
    smolMinUsize,
    smolStartsWith,
    smolEndsWith,
} from "smol_str";
```

## Core Types & Constants

```cp
class SmolStr { inline_buf: [24]u8, inline_len: u8, is_inline: bool, ext: string } // methods: empty(...) +12
```

## API Signatures

```cp
fn smolGetByte(s: SmolStr, idx: usize): u8;
fn smolMinUsize(a: usize, b: usize): usize;
fn smolStartsWith(s: SmolStr, prefix: string): bool;
fn smolEndsWith(s: SmolStr, suffix: string): bool;
```

## Minimal Usage

```cp
import { smolStartsWith } from "smol_str";

pub fn main() {
    let result = smolStartsWith(/* s: SmolStr */, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"smol_str"`; install via `cap add smol_str`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
