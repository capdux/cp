# ctr — AI Agent Quick Ref

Zero-allocation NIST SP 800-38A AES-CTR stream cipher modes (Ctr128BE / Ctr128LE) with O(1) seek in pure CP (`v1.0.0`, `MIT OR Apache-2.0`)

## Import

```cp
import {
    Aes128Ctr128Be,
    Aes128Ctr128Le,
    Aes256Ctr128Be,
    Aes256Ctr128Le,
    ctrZeroBlock,
    ctrAddBe128,
    ctrAddLe128,
} from "ctr";
```

## Core Types & Constants

```cp
class Aes128Ctr128Be { cipher: Aes128, iv: [16]u8, block_idx: u64, buf: [16]u8, /* +1 fields */ } // methods: new(...) +3
class Aes128Ctr128Le { cipher: Aes128, iv: [16]u8, block_idx: u64, buf: [16]u8, /* +1 fields */ } // methods: new(...) +3
class Aes256Ctr128Be { cipher: Aes256, iv: [16]u8, block_idx: u64, buf: [16]u8, /* +1 fields */ } // methods: new(...) +3
class Aes256Ctr128Le { cipher: Aes256, iv: [16]u8, block_idx: u64, buf: [16]u8, /* +1 fields */ } // methods: new(...) +3
```

## API Signatures

```cp
fn ctrZeroBlock(): [16]u8;
fn ctrAddBe128(iv: [16]u8, delta: u64): [16]u8;
fn ctrAddLe128(iv: [16]u8, delta: u64): [16]u8;
```

## Minimal Usage

```cp
import { ctrZeroBlock } from "ctr";

pub fn main() {
    let result = ctrZeroBlock();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ctr"`; install via `cap add ctr`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
