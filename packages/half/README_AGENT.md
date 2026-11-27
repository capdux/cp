# half — AI Agent Quick Ref

Zero-heap IEEE 754 binary16 (f16) and Google Brain Float 16 (bf16) codec and arithmetic in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    F16,
    Bf16,
    f32ToBits,
    bitsToF32,
    f32SliceToF16,
    f16SliceToF32,
} from "half";
```

## Core Types & Constants

```cp
class F16 { bits: u16 } // methods: fromBits(...) +9
class Bf16 { bits: u16 } // methods: fromBits(...) +6
```

## API Signatures

```cp
fn f32ToBits(x: f32): u32;
fn bitsToF32(b: u32): f32;
fn f32SliceToF16(src: &[]f32, dst: []u16): usize;
fn f16SliceToF32(src: &[]u16, dst: []f32): usize;
```

## Minimal Usage

```cp
import { f32ToBits } from "half";

pub fn main() {
    let result = f32ToBits(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"half"`; install via `cap add half`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
