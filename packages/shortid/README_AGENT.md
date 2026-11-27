# shortid — AI Agent Quick Ref

URL-friendly non-sequential short unique ID generator and validator (clean-room CP implementation of npm shortid) (`v1.0.0`, `MIT`)

## Import

```cp
import { ShortIdGenerator, sidU64, sidCharIndex, isValidShortId } from "shortid";
```

## Core Types & Constants

```cp
class ShortIdGenerator { alphabet: [64]u8, worker_id: u8, fmt_version: u8, last_tick: u64, /* +2 fields */ } // methods: new(...) +8
```

## API Signatures

```cp
fn sidU64(hi: u32, lo: u32): u64;
fn sidCharIndex(c: u8, alphabet: string): i32;
fn isValidShortId(id: string): bool;
```

## Minimal Usage

```cp
import { isValidShortId } from "shortid";

pub fn main() {
    let result = isValidShortId("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"shortid"`; install via `cap add shortid`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
