# glam — AI Agent Quick Ref

Zero-heap 3D math library with Vec3, unit Quat, and column-major Mat4 (Rust glam parity, pure CP with built-in sqrt/sin/cos). (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Vec3,
    Quat,
    Mat4,
    absF64,
    sqrtF64,
    sinF64,
    cosF64,
} from "glam";
```

## Core Types & Constants

```cp
class Vec3 { x: f64, y: f64, z: f64 } // methods: init(...) +10
class Quat { x: f64, y: f64, z: f64, w: f64 } // methods: identity(...) +5
class Mat4 { m: [16]f64 } // methods: identity(...) +6
```

## API Signatures

```cp
fn absF64(x: f64): f64;
fn sqrtF64(x: f64): f64;
fn sinF64(angle: f64): f64;
fn cosF64(angle: f64): f64;
```

## Minimal Usage

```cp
import { absF64 } from "glam";

pub fn main() {
    let result = absF64(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"glam"`; install via `cap add glam`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
