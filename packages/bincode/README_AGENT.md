# bincode — AI Agent Quick Ref

Zero-allocation bincode v2 compact binary serializer & zero-copy decoder (LE varint, ZigZag signed ints, length-prefixed strings, Option, fixed LE integers — Rust bincode parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    OptStrResult,
    BincodeEncoder,
    BincodeDecoder,
    zigzagEncode,
    zigzagDecode,
} from "bincode";
```

## Core Types & Constants

```cp
class OptStrResult { ok: bool, has_value: bool, value: string }
class BincodeEncoder { out: []u8, pos: usize, overflow: bool } // methods: init(...) +9
class BincodeDecoder { data: string, pos: usize } // methods: init(...) +9
```

## API Signatures

```cp
fn zigzagEncode(v: i64): u64;
fn zigzagDecode(u: u64): i64;
```

## Minimal Usage

```cp
import { zigzagEncode } from "bincode";

pub fn main() {
    let result = zigzagEncode(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"bincode"`; install via `cap add bincode`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
