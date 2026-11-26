# `@cap/perlin-noise`

Zero-heap Ken Perlin improved 1D / 2D / 3D gradient noise, Fractal Brownian Motion (`fbm2D`), and ridged multifractal noise (`ridge2D`) in pure CP.

## Features

- **100% Zero Heap Allocation**: Static 256-entry classic Perlin permutation table with deterministic `seed: u32` perturbation.
- **1D / 2D / 3D Gradient Noise**: `fade`, `noise1D`, `noise2D`, `noise3D` with exact zero crossings on integer lattice points.
- **Fractal Octaves**: `fbm2D` (normalized to `[-1, 1]`) and `ridge2D` (`1 - |n|`, normalized to `[0, 1]`).

## Quick Start

```cp
import { noise2D, fbm2D, ridge2D } from "@cap/perlin-noise";

const n = noise2D(1.25, 3.75, 42);
const terrain = fbm2D(0.5, 0.5, 42, 5, 2.0, 0.5);
const mountain = ridge2D(0.5, 0.5, 42, 4);
```

## License

MIT
