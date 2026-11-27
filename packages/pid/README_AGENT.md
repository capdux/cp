# pid@1.0.0 (Agent Quickref)

> Zero-allocation industrial PID feedback controller with term limits, anti-windup, and derivative-on-measurement in pure CP | License: MIT
> Import: `import { ... } from "pid";` | Full types: `src/index.d.cp`

## Functions
- `export fn clampRange(v: f64, lo: f64, hi: f64): f64;`
- `export fn clampSymmetric(v: f64, lim: f64): f64;`
