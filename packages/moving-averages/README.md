[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# moving-averages

Zero-heap streaming and batch moving averages (`MA`/`SMA`, `EMA`, `DMA`, `SMMA`, `WMA`, `CMA`) in pure CP.

- **Upstream Alignment**: Combines npm [`moving-averages`](https://github.com/kaelzhang/node-moving-averages) (`MIT`) batch series functions with Rust [`simple_moving_average`](https://github.com/emilec/simple_moving_average) (`crates.io`, `MIT`) streaming calculators.
- **Package Version**: `1.0.0`

## Installation & Import

```bash
cap add moving-averages
```

In `cap.json`:

```json
{
  "dependencies": {
    "moving-averages": "^1.0.0"
  }
}
```

```typescript
import {
    Ema, Sma, Cma, Wma,
    ma, sma, dma, ema, smma, wma, cma,
    smaSlice, wmaSlice, maNan
} from "moving-averages";
```

## Core API Reference

### Streaming Calculators (Zero-Heap)

| Class | Methods | Description |
| :--- | :--- | :--- |
| `Ema` | `Ema.new(alpha: f64)`, `Ema.fromPeriod(period: u32)`, `update(*_, sample: f64): f64`, `reset(*_)` | Exponential Moving Average (`alpha = 2 / (period + 1)`) |
| `Sma` | `Sma.new()`, `update(*_, window: []f64, sample: f64): f64`, `average(): f64`, `reset(*_)` | O(1) sliding Simple Moving Average backed by caller ring buffer `window` |
| `Cma` | `Cma.new()`, `update(*_, sample: f64): f64`, `average(): f64`, `reset(*_)` | Cumulative Moving Average over all samples seen so far |
| `Wma` | `Wma.new()`, `update(*_, window: []f64, sample: f64): f64`, `reset(*_)` | Linearly Weighted Moving Average backed by caller ring buffer `window` |

### Batch Series & Slice Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `ma` / `sma` | `ma(data: &[]f64, size: usize, out: []f64): usize` | Simple moving average series (leading `size - 1` slots filled with `maNan()`) |
| `dma` | `dma(data: &[]f64, alpha: f64, out: []f64): usize` | Dynamic moving average series with smoothing factor `alpha` |
| `ema` | `ema(data: &[]f64, size: usize, out: []f64): usize` | Exponential moving average series (`alpha = 2 / (size + 1)`) |
| `smma` | `smma(data: &[]f64, size: usize, out: []f64): usize` | Smoothed (Wilder's) moving average series (`alpha = 1 / size`) |
| `wma` | `wma(data: &[]f64, size: usize, out: []f64): usize` | Linearly weighted moving average series |
| `cma` | `cma(data: &[]f64, out: []f64): usize` | Cumulative moving average series |
| `smaSlice` / `wmaSlice` | `smaSlice(xs: []f64): f64` / `wmaSlice(xs: []f64): f64` | One-shot scalar SMA / WMA over a slice |
| `maNan` | `maNan(): f64` | Returns quiet IEEE-754 `NaN` used for warm-up positions |

## Examples

### 1. Streaming `Sma` and `Ema` Over Live Telemetry

```typescript
import { Sma, Ema } from "moving-averages";

fn trackLatency(): f64 {
    let mut win: [4]f64 = [0.0, 0.0, 0.0, 0.0];
    let mut sma_calc = Sma.new();
    let mut ema_calc = Ema.fromPeriod(5);

    let samples = [10.0, 20.0, 30.0, 40.0, 50.0];
    let mut last_ema: f64 = 0.0;
    for (s in samples) {
        let _ = sma_calc.update(&mut win, s);
        last_ema = ema_calc.update(s);
    }
    return sma_calc.average() + last_ema;
}
```

### 2. Batch `ema` & `wma` Series Computation

```typescript
import { ema, wma } from "moving-averages";

fn computeIndicators(prices: &[]f64, ema_out: []f64, wma_out: []f64): usize {
    let n1 = ema(prices, 3, ema_out);
    let n2 = wma(prices, 3, wma_out);
    return if (n1 < n2) { n1 } else { n2 };
}
```

## Differences from Upstream & CP Highlights

- **Caller-Owned Ring Buffers**: Streaming `Sma` and `Wma` take a caller-provided `window: []f64` slice of any period length, eliminating dynamic heap allocations.
- **Zero-Allocation Batch API**: `ma`, `sma`, `dma`, `ema`, `smma`, `wma`, and `cma` read `&[]f64` and write results directly into `out: []f64`, returning the number of elements written.
- **Unified Streaming + Batch Ergonomics**: Bridges both npm `moving-averages` batch semantics (`NaN` warm-up prefix via `maNan()`) and Rust `simple_moving_average` online updates.
