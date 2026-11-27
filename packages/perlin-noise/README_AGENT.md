# perlin-noise — AI Agent Quick Ref

Zero-heap Ken Perlin improved 1D/2D/3D gradient noise, fBm, and ridge noise in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    floorF64,
    permAt,
    lerpF64,
    grad1D,
    grad2D,
    grad3D,
    fade,
    noise1D,
} from "perlin-noise";
```

## API Signatures

```cp
fn floorF64(x: f64): i64;
fn permAt(idx: usize, seed: u32): usize;
fn lerpF64(a: f64, b: f64, t: f64): f64;
fn grad1D(hash: usize, x: f64): f64;
fn grad2D(hash: usize, x: f64, y: f64): f64;
fn grad3D(hash: usize, x: f64, y: f64, z: f64): f64;
fn fade(t: f64): f64;
fn noise1D(x: f64, seed: u32): f64;
fn noise2D(x: f64, y: f64, seed: u32): f64;
fn noise3D(x: f64, y: f64, z: f64, seed: u32): f64;
fn fbm2D(x: f64, y: f64, seed: u32, octaves: u32, lacunarity: f64, gain: f64): f64;
fn ridge2D(x: f64, y: f64, seed: u32, octaves: u32): f64;
```

## Minimal Usage

```cp
import { floorF64 } from "perlin-noise";

pub fn main() {
    let result = floorF64(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"perlin-noise"`; install via `cap add perlin-noise`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
