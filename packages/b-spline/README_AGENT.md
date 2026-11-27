# b-spline@1.0.0 (Agent Quickref)

> CP package b-spline | License: MIT
> Import: `import { ... } from "b-spline";` | Full types: `src/index.d.cp`

## Functions
- `export fn hermite1D(p0: f64, m0: f64, p1: f64, m1: f64, t: f64): f64;`
- `export fn hermite2D(p0: Pt2, m0: Pt2, p1: Pt2, m1: Pt2, t: f64): Pt2;`
- `export fn catmullRom1D(p0: f64, p1: f64, p2: f64, p3: f64, t: f64): f64;`
- `export fn catmullRom2D(p0: Pt2, p1: Pt2, p2: Pt2, p3: Pt2, t: f64): Pt2;`
- `export fn uniformCubicBSpline1D(p0: f64, p1: f64, p2: f64, p3: f64, t: f64): f64;`
- `export fn uniformCubicBSpline2D(p0: Pt2, p1: Pt2, p2: Pt2, p3: Pt2, t: f64): Pt2;`
- `export fn evalCatmullRomChain(pts: &[]Pt2, u: f64): Pt2;`
