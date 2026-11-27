# vec_map — AI Agent Quick Ref

Zero-allocation O(1) direct-indexed sparse integer-key map (0..127) with 128-bit presence bitmap in CP (`v1.0.0`, `MIT OR Apache-2.0`)

## Import

```cp
import { VecMapI64, vmapMask, vmapHasBit } from "vec_map";
```

## Core Types & Constants

```cp
class VecMapI64 { slots: [128]i64, bits_lo: u64, bits_hi: u64, count: usize } // methods: new(...) +10
```

## API Signatures

```cp
fn vmapMask(bit: usize): u64;
fn vmapHasBit(bits_lo: u64, bits_hi: u64, k: usize): bool;
```

## Minimal Usage

```cp
import { vmapMask } from "vec_map";

pub fn main() {
    let result = vmapMask(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"vec_map"`; install via `cap add vec_map`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add vec_map
cpc check src/main.cp
```
