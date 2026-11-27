# d3-array@1.0.0 (Agent Quickref)

> Zero-allocation array statistics, R-7 quantiles, median, variance, deviation, bisection search, and histogram binning (clean-room MIT implementation of npm d3-array, #345 / 153M wk) | License: MIT
> Import: `import { ... } from "d3-array";` | Full types: `src/index.d.cp`

## Functions
- `export fn sumF64(data: &[]f64): f64;`
- `export fn meanF64(data: &[]f64): f64?;`
- `export fn extentF64(data: &[]f64): ExtentF64;`
- `export fn varianceF64(data: &[]f64): f64?;`
- `export fn deviationF64(data: &[]f64): f64?;`
- `export fn quantileSorted(sorted: &[]f64, p: f64): f64?;`
- `export fn medianSorted(sorted: &[]f64): f64?;`
- `export fn bisectLeft(sorted: &[]f64, x: f64): usize;`
- `export fn bisectRight(sorted: &[]f64, x: f64): usize;`
- `export fn cumsumF64(data: &[]f64, out: []f64): usize;`
- `export fn binCounts(data: &[]f64, domain_min: f64, domain_max: f64, out_counts: []usize): usize;`
