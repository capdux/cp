# simple-statistics@1.0.0 (Agent Quickref)

> Zero-heap Welford online statistics, Chan parallel merge, descriptive slice stats, covariance, Pearson correlation, and linear-interpolated percentiles. | License: MIT
> Import: `import { ... } from "simple-statistics";` | Full types: `src/index.d.cp`

## Functions
- `export fn absF64(x: f64): f64;`
- `export fn sqrtF64(x: f64): f64;`
- `export fn sum(xs: &[]f64): f64;`
- `export fn mean(xs: &[]f64): f64;`
- `export fn variance(xs: &[]f64): f64;`
- `export fn stddev(xs: &[]f64): f64;`
- `export fn minSlice(xs: &[]f64): f64;`
- `export fn maxSlice(xs: &[]f64): f64;`
- `export fn covariance(xs: &[]f64, ys: &[]f64): f64;`
- `export fn pearsonR(xs: &[]f64, ys: &[]f64): f64;`
- `export fn percentileSorted(sorted: &[]f64, p: f64): f64;`
- `export fn medianSorted(sorted: &[]f64): f64;`
