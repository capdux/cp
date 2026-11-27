# moving-averages — AI Agent Quick Ref

Zero-heap streaming (`Ema`, `Sma`, `Cma`, `Wma`) and batch (`ma`/`sma`, `dma`, `ema`, `smma`, `wma`, `cma`) moving averages in pure CP.

## Import
```typescript
import {
    Ema, Sma, Cma, Wma,
    ma, sma, dma, ema, smma, wma, cma,
    smaSlice, wmaSlice, maNan
} from "moving-averages";
```

## API Signatures
```typescript
export fn maNan(): f64;
export class Ema {
    alpha: f64, value: f64, initialized: bool,
    fn new(alpha: f64): Ema;
    fn fromPeriod(period: u32): Ema;
    fn update(*_, sample: f64): f64;
    fn reset(*_);
}
export class Sma {
    head: usize, count: usize, sum: f64,
    fn new(): Sma;
    fn update(*_, window: []f64, sample: f64): f64;
    fn average(): f64;
    fn reset(*_);
}
export class Cma {
    count: usize, value: f64,
    fn new(): Cma;
    fn update(*_, sample: f64): f64;
    fn average(): f64;
    fn reset(*_);
}
export class Wma {
    head: usize, count: usize,
    fn new(): Wma;
    fn update(*_, window: []f64, sample: f64): f64;
    fn reset(*_);
}
export fn smaSlice(xs: []f64): f64;
export fn wmaSlice(xs: []f64): f64;
export fn ma(data: &[]f64, size: usize, out: []f64): usize;
export fn sma(data: &[]f64, size: usize, out: []f64): usize;
export fn dma(data: &[]f64, alpha: f64, out: []f64): usize;
export fn ema(data: &[]f64, size: usize, out: []f64): usize;
export fn smma(data: &[]f64, size: usize, out: []f64): usize;
export fn wma(data: &[]f64, size: usize, out: []f64): usize;
export fn cma(data: &[]f64, out: []f64): usize;
```

## Minimal Example
```typescript
import { Sma, ema } from "moving-averages";

fn demo(): f64 {
    let mut win: [3]f64 = [0.0, 0.0, 0.0];
    let mut s = Sma.new();
    let _ = s.update(&mut win, 10.0);
    let avg = s.update(&mut win, 20.0); // 15.0
    let data = [1.0, 2.0, 3.0, 4.0];
    let mut out: [4]f64 = undefined;
    let _ = ema(&data, 2, &mut out);
    return avg + out[3];
}
```

## Pitfalls & Rules
- `Sma.update` and `Wma.update` mutate the caller-owned `window: []f64` ring buffer; pass the same slice across calls and bind the calculator with `let mut`.
- Batch functions take read-only `data: &[]f64` and write up to `min(data.len, out.len)` values into `out: []f64`.
- Warm-up slots (indices `0..size-1` in `ma`/`sma`/`wma`) are filled with `maNan()` (IEEE-754 `NaN`).
- `ma` and `sma` are aliases; `Ema.fromPeriod(p)` uses `alpha = 2.0 / (p + 1)`.
