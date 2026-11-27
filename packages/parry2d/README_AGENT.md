# parry2d — AI Agent Quick Ref

Zero-allocation 2D axis-aligned bounding box (AABB) collision detection, union, intersection, and ray slab intersection for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Pt2,
    Aabb2,
    minF64,
    maxF64,
    new,
    fromCenterSize,
    fromPoints,
} from "parry2d";
```

## Core Types & Constants

```cp
class Pt2 { x: f64, y: f64 } // methods: new(...)
class Aabb2 { min_x: f64, min_y: f64, max_x: f64, max_y: f64 } // methods: new(...) +18
```

## API Signatures

```cp
fn minF64(a: f64, b: f64): f64;
fn maxF64(a: f64, b: f64): f64;
fn new(min_x: f64, min_y: f64, max_x: f64, max_y: f64): Aabb2;
fn fromCenterSize(cx: f64, cy: f64, w: f64, h: f64): Aabb2;
fn fromPoints(pts: &[]Pt2): Aabb2;
```

## Minimal Usage

```cp
import { minF64 } from "parry2d";

pub fn main() {
    let result = minF64(0.0, 0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"parry2d"`; install via `cap add parry2d`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
