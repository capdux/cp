# average@1.0.0 (Agent Quickref)

> Zero-allocation Welford streaming statistics, Chan parallel merge, OLS linear regression, and linear-interpolated percentiles for CP | License: MIT
> Import: `import { ... } from "average";` | Full types: `src/index.d.cp`

## Functions
- `export fn linearRegression(xs: &[]f64, ys: &[]f64): LinearFit;`
- `export fn percentile(values: &[]f64, p: f64): f64?;`
- `export fn median(values: &[]f64): f64?;`
- `export fn iqr(values: &[]f64): f64?;`
