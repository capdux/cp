# moving-averages — AI Agent Quick Ref

- **Import**: `import { Ema, Sma, Cma, Wma, ma, sma, dma, ema, smma, wma, cma, smaSlice, wmaSlice, maNan } from "moving-averages";`
- **Source**: npm `moving-averages` + crates.io `simple_moving_average` (`v1.0.0`, zero-heap)

## Core Signatures
```cp
// Batch series (write into caller-provided `out`, return elements written = min(data.len, out.len))
ma(data: &[]f64, size: usize, out: []f64): usize;    // alias: sma(...)
dma(data: &[]f64, alpha: f64, out: []f64): usize;    // 0 < alpha <= 1
ema(data: &[]f64, size: usize, out: []f64): usize;   // alpha = 2 / (size + 1)
smma(data: &[]f64, size: usize, out: []f64): usize;  // alpha = 1 / size
wma(data: &[]f64, size: usize, out: []f64): usize;   // linear weights 1..size
cma(data: &[]f64, out: []f64): usize;                // cumulative average
smaSlice(xs: []f64): f64;
wmaSlice(xs: []f64): f64;
maNan(): f64;

// Streaming estimators
class Ema {
    alpha: f64, value: f64, initialized: bool,
    static new(alpha: f64): Ema;
    static fromPeriod(period: u32): Ema;
    update(sample: f64): f64;
    reset();
}
class Sma {
    head: usize, count: usize, sum: f64,
    static new(): Sma;
    update(window: []f64, sample: f64): f64;
    average(): f64;
    reset();
}
class Wma {
    head: usize, count: usize,
    static new(): Wma;
    update(window: []f64, sample: f64): f64;
    reset();
}
class Cma {
    count: usize, value: f64,
    static new(): Cma;
    update(sample: f64): f64;
    average(): f64;
    reset();
}
```

## Pitfalls
- For `ma`/`sma`/`wma`, indices `0 .. size-2` are warm-up slots filled with `NaN` (`maNan()`), matching npm `moving-averages`.
- `Sma` and `Wma` do not allocate: always pass the same backing `window: []f64` buffer on every `update` call.
- Bind streaming estimators with `let mut` since `update`/`reset` mutate fields.
- Ensure `out.len >= data.len` for full batch output; if `size == 0`, batch functions return `0`.
