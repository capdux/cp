# d3-scale@1.0.0 (Agent Quickref)

> Zero-allocation D3 quantitative scales (LinearScale, PowScale, SqrtScale, LogScale, QuantizeScale) in pure CP | License: ISC
> Import: `import { ... } from "d3-scale";` | Full types: `src/index.d.cp`

## Functions
- `export fn d3scClamp01(t: f64): f64;`
- `export fn d3scLerp(a: f64, b: f64, t: f64): f64;`
- `export fn d3scSignedPow(x: f64, exp: f64): f64;`
- `export fn d3scTickIncrement(start: f64, stop: f64, count: usize): f64;`
- `export fn linearScale(d0: f64, d1: f64, r0: f64, r1: f64): LinearScale;`
- `export fn powScale(d0: f64, d1: f64, r0: f64, r1: f64, exponent: f64): PowScale;`
- `export fn sqrtScale(d0: f64, d1: f64, r0: f64, r1: f64): SqrtScale;`
- `export fn logScale(d0: f64, d1: f64, r0: f64, r1: f64, base: f64): LogScale;`
- `export fn quantizeScale(d0: f64, d1: f64, buckets: usize): QuantizeScale;`
