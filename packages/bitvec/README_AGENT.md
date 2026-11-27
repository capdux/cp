# bitvec — AI Agent Quick Ref

Zero-heap arbitrary-length bit vector with popcount, find-first-one, bitwise ops and shifts (clean-room CP implementation of crates.io bitvec) (`v1.0.0`, `MIT`)

## Import

```cp
import { BitVec512, bvPopcountU64 } from "bitvec";
```

## Core Types & Constants

```cp
class BitVec512 { words: [8]u64, bit_len: usize } // methods: new(...) +21
```

## API Signatures

```cp
fn bvPopcountU64(x: u64): usize;
```

## Minimal Usage

```cp
import { bvPopcountU64 } from "bitvec";

pub fn main() {
    let result = bvPopcountU64(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"bitvec"`; install via `cap add bitvec`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add bitvec
cpc check src/main.cp
```
