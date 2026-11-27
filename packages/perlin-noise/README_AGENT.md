# perlin-noise@1.0.0 (Agent Quickref)

> CP package perlin-noise | License: MIT
> Import: `import { ... } from "perlin-noise";` | Full types: `src/index.d.cp`

## Functions
- `export fn floorF64(x: f64): i64;`
- `export fn permAt(idx: usize, seed: u32): usize;`
- `export fn lerpF64(a: f64, b: f64, t: f64): f64;`
- `export fn grad1D(hash: usize, x: f64): f64;`
- `export fn grad2D(hash: usize, x: f64, y: f64): f64;`
- `export fn grad3D(hash: usize, x: f64, y: f64, z: f64): f64;`
- `export fn fade(t: f64): f64;`
- `export fn noise1D(x: f64, seed: u32): f64;`
- `export fn noise2D(x: f64, y: f64, seed: u32): f64;`
- `export fn noise3D(x: f64, y: f64, z: f64, seed: u32): f64;`
- `export fn fbm2D(x: f64, y: f64, seed: u32, octaves: u32, lacunarity: f64, gain: f64): f64;`
- `export fn ridge2D(x: f64, y: f64, seed: u32, octaves: u32): f64;`
