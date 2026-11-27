# ecdsa-sig-formatter — AI Agent Quick Ref

Zero-allocation ASN.1 DER <-> JOSE IEEE P1363 (r || s) ECDSA signature converter for ES256, ES384, and ES512 in pure CP (`v1.0.0`, `Apache-2.0`)

## Import

```cp
import {
    EcdsaAlg,
    paramBytes,
    esfParseDerInt,
    esfTrimLeadingZeros,
    derToJose,
    joseToDer,
} from "ecdsa-sig-formatter";
```

## Core Types & Constants

```cp
enum EcdsaAlg { ES256, ES384, ES512 }
```

## API Signatures

```cp
fn paramBytes(alg: EcdsaAlg): usize;
fn esfParseDerInt(der: &[]u8, pos: usize, limit: usize, param_len: usize, dst: []u8): i64;
fn esfTrimLeadingZeros(src: &[]u8): usize;
fn derToJose(der: &[]u8, alg: EcdsaAlg, out: []u8): i64;
fn joseToDer(jose: &[]u8, alg: EcdsaAlg, out: []u8): i64;
```

## Minimal Usage

```cp
import { paramBytes } from "ecdsa-sig-formatter";

pub fn main() {
    let result = paramBytes(/* alg: EcdsaAlg */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ecdsa-sig-formatter"`; install via `cap add ecdsa-sig-formatter`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
