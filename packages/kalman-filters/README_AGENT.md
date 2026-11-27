# kalman-filters — AI Agent Quick Ref

Zero-allocation 1D scalar and 2D constant-velocity Kalman filters with Joseph-form covariance update (Rust kalman-filters / kfilter parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { Kalman1D, Kalman2D } from "kalman-filters";
```

## Core Types & Constants

```cp
class Kalman1D { x: f64, p: f64, q: f64, r: f64, /* +1 fields */ } // methods: init(...) +7
class Kalman2D { pos: f64, vel: f64, p00: f64, p01: f64, /* +5 fields */ } // methods: init(...) +4
```

## Minimal Usage

```cp
import { Kalman1D } from "kalman-filters";

pub fn main() {
    // Refer to src/index.d.cp for Kalman1D fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"kalman-filters"`; install via `cap add kalman-filters`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add kalman-filters
cpc check src/main.cp
```
