# enumflags2 — AI Agent Quick Ref

Zero-allocation 64-bit type-safe enum bitflags set with valid-mask protection for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import { BitFlags64, ef2Popcount64, ef2Ctz64 } from "enumflags2";
```

## Core Types & Constants

```cp
class BitFlags64 { bits: u64, valid_mask: u64 } // methods: empty(...) +20
```

## API Signatures

```cp
fn ef2Popcount64(val: u64): usize;
fn ef2Ctz64(val: u64): u8;
```

## Minimal Usage

```cp
import { ef2Popcount64 } from "enumflags2";

pub fn main() {
    let result = ef2Popcount64(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"enumflags2"`; install via `cap add enumflags2`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add enumflags2
cpc check src/main.cp
```
