# fixedbitset — AI Agent Quick Ref

Zero-allocation fixed-size bitsets BitSet256/BitSet1024 with SWAR popcount, fast scanning, and set algebra (Rust fixedbitset / bit-set parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    BitSet256,
    BitSet1024,
    allOnes64,
    bitMask64,
    popcount64,
    ctz64,
} from "fixedbitset";
```

## Core Types & Constants

```cp
class BitSet256 { words: [4]u64 } // methods: init(...) +19
class BitSet1024 { words: [16]u64 } // methods: init(...) +19
```

## API Signatures

```cp
fn allOnes64(): u64;
fn bitMask64(bit: usize): u64;
fn popcount64(x_in: u64): usize;
fn ctz64(w_in: u64): usize;
```

## Minimal Usage

```cp
import { allOnes64 } from "fixedbitset";

pub fn main() {
    let result = allOnes64();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"fixedbitset"`; install via `cap add fixedbitset`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
