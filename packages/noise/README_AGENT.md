# noise@1.0.0 (Agent Quickref)

> Zero-heap 2D/3D Improved Perlin Noise, Value Noise, and Fractal Brownian Motion (fBm) in pure CP | License: MIT
> Import: `import { ... } from "noise";` | Full types: `src/index.d.cp`

## Functions
- `export fn floorInt(x: f64): i64;`
- `export fn fade(t: f64): f64;`
- `export fn lerp(a: f64, b: f64, t: f64): f64;`
- `export fn grad2(hash: u8, x: f64, y: f64): f64;`
- `export fn grad3(hash: u8, x: f64, y: f64, z: f64): f64;`
- `export fn splitmix64(state: &mut u64): u64;`
