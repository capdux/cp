# parry3d — AI Agent Quick Ref

Zero-allocation 3D Axis-Aligned Bounding Box (AABB) collision detection, union/intersection, surface area/volume, and Kay-Kajiya slab raycast library for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Pt3,
    Aabb3,
    abAbs,
    abMin,
    abMax,
} from "parry3d";
```

## Core Types & Constants

```cp
class Pt3 { x: f64, y: f64, z: f64 }
class Aabb3 { min_x: f64, min_y: f64, min_z: f64, max_x: f64, /* +2 fields */ } // methods: new(...) +12
```

## API Signatures

```cp
fn abAbs(x: f64): f64;
fn abMin(a: f64, b: f64): f64;
fn abMax(a: f64, b: f64): f64;
```

## Minimal Usage

```cp
import { abAbs } from "parry3d";

pub fn main() {
    let result = abAbs(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"parry3d"`; install via `cap add parry3d`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
