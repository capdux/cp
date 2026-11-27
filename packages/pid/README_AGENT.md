# pid — AI Agent Quick Ref

Zero-allocation industrial PID feedback controller with term limits, anti-windup, and derivative-on-measurement in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import { PidOutput, PidController, clampRange, clampSymmetric } from "pid";
```

## Core Types & Constants

```cp
class PidOutput { output: f64, p: f64, i: f64, d: f64 }
class PidController { setpoint: f64, kp: f64, ki: f64, kd: f64, /* +9 fields */ } // methods: init(...) +7
```

## API Signatures

```cp
fn clampRange(v: f64, lo: f64, hi: f64): f64;
fn clampSymmetric(v: f64, lim: f64): f64;
```

## Minimal Usage

```cp
import { clampRange } from "pid";

pub fn main() {
    let result = clampRange(0.0, 0.0, 0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"pid"`; install via `cap add pid`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
