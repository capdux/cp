# curve25519-dalek — AI Agent Quick Ref

Zero-allocation constant-time RFC 7748 X25519 elliptic curve Diffie-Hellman and GF(2^255-19) field arithmetic (curve25519-dalek parity) for CP (`v1.0.0`, `BSD-3-Clause`)

## Import

```cp
import {
    FieldElement,
    c25Carry,
    c25Sel,
    clampScalar,
    x25519,
    x25519Basepoint,
} from "curve25519-dalek";
```

## Core Types & Constants

```cp
class FieldElement { limbs: [16]i64 } // methods: fromBytes(...) +6
```

## API Signatures

```cp
fn c25Carry(o: []i64);
fn c25Sel(p: []i64, q: []i64, b: i64);
fn clampScalar(k: [32]u8): [32]u8;
fn x25519(scalar: [32]u8, u_coord: [32]u8): [32]u8;
fn x25519Basepoint(scalar: [32]u8): [32]u8;
```

## Minimal Usage

```cp
import { clampScalar } from "curve25519-dalek";

pub fn main() {
    let result = clampScalar(/* k: [32]u8 */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"curve25519-dalek"`; install via `cap add curve25519-dalek`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
