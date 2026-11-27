# moving-averages@1.0.0 (Agent Quickref)

> Zero-heap Exponential Moving Average (EMA), Simple Moving Average (SMA), and Weighted Moving Average (WMA) in pure CP | License: MIT
> Import: `import { ... } from "moving-averages";` | Full types: `src/index.d.cp`

## Functions
- `export fn maNan(): f64;`
- `export fn smaSlice(xs: []f64): f64;`
- `export fn wmaSlice(xs: []f64): f64;`
- `export fn ma(data: &[]f64, size: usize, out: []f64): usize;`
- `export fn sma(data: &[]f64, size: usize, out: []f64): usize;`
- `export fn dma(data: &[]f64, alpha: f64, out: []f64): usize;`
- `export fn ema(data: &[]f64, size: usize, out: []f64): usize;`
- `export fn smma(data: &[]f64, size: usize, out: []f64): usize;`
- `export fn wma(data: &[]f64, size: usize, out: []f64): usize;`
- `export fn cma(data: &[]f64, out: []f64): usize;`
