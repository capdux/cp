# xorf — AI Agent Quick Ref

Zero-allocation Xor8 static fingerprint filter (Graf & Lemire ACM JEA 2020 / Rust xorf parity) with O(1) 3-lookup XOR queries and ~0.39% false-positive rate for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Slots3,
    Xor8Filter,
    makeU64,
    hashKey64,
    deriveSlots,
} from "xorf";
```

## Core Types & Constants

```cp
class Slots3 { h0: usize, h1: usize, h2: usize, fp: u8 }
class Xor8Filter { seed: u64, block_length: usize, fingerprints: [MAX_SLOTS]u8, key_count: usize, /* +1 fields */ } // methods: emptyWithStatus(...) +5
```

## API Signatures

```cp
fn makeU64(hi: u32, lo: u32): u64;
fn hashKey64(key: u64, seed: u64): u64;
fn deriveSlots(h: u64, block_len: usize): Slots3;
```

## Minimal Usage

```cp
import { hashKey64 } from "xorf";

pub fn main() {
    let result = hashKey64(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"xorf"`; install via `cap add xorf`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
