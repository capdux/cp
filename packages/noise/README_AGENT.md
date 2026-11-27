# noise — AI Agent Quick Ref

Zero-heap 2D/3D Improved Perlin Noise, Value Noise, and Fractal Brownian Motion (fBm) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PerlinNoise,
    floorInt,
    fade,
    lerp,
    grad2,
    grad3,
    splitmix64,
} from "noise";
```

## Core Types & Constants

```cp
class PerlinNoise { perm: [512]u8 } // methods: init(...) +4
```

## API Signatures

```cp
fn floorInt(x: f64): i64;
fn fade(t: f64): f64;
fn lerp(a: f64, b: f64, t: f64): f64;
fn grad2(hash: u8, x: f64, y: f64): f64;
fn grad3(hash: u8, x: f64, y: f64, z: f64): f64;
fn splitmix64(state: &mut u64): u64;
```

## Minimal Usage

```cp
import { floorInt } from "noise";

pub fn main() {
    let result = floorInt(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"noise"`; install via `cap add noise`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
