[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# moving-averages

Zero-heap moving averages in pure CP: both **batch series APIs** (`ma`, `sma`, `dma`, `ema`, `smma`, `wma`, `cma` — full npm `moving-averages` parity) and **streaming O(1) stateful estimators** (`Ema`, `Sma`, `Cma`, `Wma`).

## Provenance & Zero-Friction Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | npm (+ crates.io `simple_moving_average`) |
| **Upstream Package** | `moving-averages` (`kaelzhang/node-moving-averages`) |
| **CP Package** | `moving-averages` (`v1.0.0`) |
| **License** | MIT |

Keep the exact package name in CP — no `@cap/*` prefix or renaming required:

```typescript
// TypeScript / Node.js (npm: moving-averages)
import { ma, dma, ema, smma, wma } from "moving-averages";
const out = ema([1, 2, 3, 4, 5], 3);
```

```typescript
// CP (same package name: "moving-averages")
import { ma, dma, ema, smma, wma } from "moving-averages";
let data: [5]f64 = [1.0, 2.0, 3.0, 4.0, 5.0];
let mut out: [5]f64 = undefined;
let n = ema(data[0..5], 3, out[0..5]);
```

## Installation

```bash
cap add moving-averages
```

## Quick Start & Practical Examples

### 1. Batch Series APIs (Full npm `moving-averages` Parity)

```typescript
import { ma, sma, dma, ema, smma, wma, cma, smaSlice, wmaSlice } from "moving-averages";

fn computeSeries(): void {
    let prices: [6]f64 = [10.0, 12.0, 11.0, 14.0, 15.0, 16.0];
    let mut out: [6]f64 = undefined;

    // Simple Moving Average (`sma` is an alias for `ma`)
    let n1 = ma(prices[0..6], 3, out[0..6]);
    let n2 = sma(prices[0..6], 3, out[0..6]);

    // Dynamic Moving Average with custom alpha (0 < alpha <= 1)
    let n3 = dma(prices[0..6], 0.5, out[0..6]);

    // Exponential Moving Average (alpha = 2 / (size + 1))
    let n4 = ema(prices[0..6], 3, out[0..6]);

    // Smoothed Moving Average (Wilder's SMMA, alpha = 1 / size)
    let n5 = smma(prices[0..6], 3, out[0..6]);

    // Linearly Weighted Moving Average & Cumulative Moving Average
    let n6 = wma(prices[0..6], 3, out[0..6]);
    let n7 = cma(prices[0..6], out[0..6]);

    // Single-window slice summaries
    let s_avg = smaSlice(prices[0..6]);
    let w_avg = wmaSlice(prices[0..6]);
}
```

### 2. Zero-Heap Streaming Estimators (`Ema`, `Sma`, `Wma`, `Cma`)

```typescript
import { Ema, Sma, Wma, Cma } from "moving-averages";

fn streamingMetrics(): void {
    // Exponential Moving Average (by alpha or period)
    let mut ema_est = Ema.fromPeriod(5); // or Ema.new(0.2)
    let e1 = ema_est.update(100.0);
    let e2 = ema_est.update(105.0);
    ema_est.reset();

    // Ring-buffered Simple Moving Average (caller owns the stack window)
    let mut sma_buf: [4]f64 = [0.0, 0.0, 0.0, 0.0];
    let mut sma_est = Sma.new();
    let s1 = sma_est.update(sma_buf[0..4], 10.0);
    let s2 = sma_est.update(sma_buf[0..4], 20.0);
    let cur = sma_est.average();

    // Ring-buffered Weighted Moving Average
    let mut wma_buf: [4]f64 = [0.0, 0.0, 0.0, 0.0];
    let mut wma_est = Wma.new();
    let w1 = wma_est.update(wma_buf[0..4], 10.0);
    let w2 = wma_est.update(wma_buf[0..4], 20.0);

    // Cumulative Moving Average
    let mut cma_est = Cma.new();
    let c1 = cma_est.update(10.0);
    let c2 = cma_est.update(20.0); // 15.0
    let c_avg = cma_est.average();
}
```

## API Reference

### Batch Series Functions
- `ma(data: &[]f64, size: usize, out: []f64): usize` / `sma(data: &[]f64, size: usize, out: []f64): usize`
- `dma(data: &[]f64, alpha: f64, out: []f64): usize`
- `ema(data: &[]f64, size: usize, out: []f64): usize`
- `smma(data: &[]f64, size: usize, out: []f64): usize`
- `wma(data: &[]f64, size: usize, out: []f64): usize`
- `cma(data: &[]f64, out: []f64): usize`
- `smaSlice(xs: []f64): f64`, `wmaSlice(xs: []f64): f64`, `maNan(): f64`

### Streaming Classes
- `Ema`: `Ema.new(alpha: f64)`, `Ema.fromPeriod(period: u32)`, `update(sample: f64): f64`, `reset()`
- `Sma`: `Sma.new()`, `update(window: []f64, sample: f64): f64`, `average(): f64`, `reset()`
- `Wma`: `Wma.new()`, `update(window: []f64, sample: f64): f64`, `reset()`
- `Cma`: `Cma.new()`, `update(sample: f64): f64`, `average(): f64`, `reset()`
