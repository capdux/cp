# wildmatch — AI Agent Quick Ref

Zero-heap linear-time wildcard (*, ?, custom wildcards & case-insensitive) pattern matcher (clean-room port of Rust wildmatch) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    WildMatch,
    wmByteEq,
    wmMatchCore,
    wildmatch,
    wildmatchIgnoreCase,
    wildmatchCustom,
    hasWildcards,
} from "wildmatch";
```

## Core Types & Constants

```cp
class WildMatch { pattern: string, any_many: u8, any_one: u8, ignore_case: bool, /* +1 fields */ } // methods: new(...) +4
```

## API Signatures

```cp
fn wmByteEq(a: u8, b: u8, ignore_case: bool): bool;
fn wmMatchCore(pattern: string, input: string, any_many: u8, any_one: u8, ignore_case: bool, allow_escape: bool): bool;
fn wildmatch(pattern: string, input: string): bool;
fn wildmatchIgnoreCase(pattern: string, input: string): bool;
fn wildmatchCustom(pattern: string, input: string, any_many: u8, any_one: u8): bool;
fn hasWildcards(pattern: string): bool;
```

## Minimal Usage

```cp
import { wildmatch } from "wildmatch";

pub fn main() {
    let result = wildmatch("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"wildmatch"`; install via `cap add wildmatch`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
