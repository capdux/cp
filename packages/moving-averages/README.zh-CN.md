[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# moving-averages

零堆分配的纯 CP 移动平均算法库：同时提供 **批量序列 API**（`ma`、`sma`、`dma`、`ema`、`smma`、`wma`、`cma`，完整对齐 npm `moving-averages`）与 **流式 O(1) 有状态估计器**（`Ema`、`Sma`、`Cma`、`Wma`）。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源平台** | npm（并参考 crates.io `simple_moving_average`） |
| **上游原包** | `moving-averages` (`kaelzhang/node-moving-averages`) |
| **CP 包名** | `moving-averages` (`v1.0.0`) |
| **许可证** | MIT |

CP 生态保持原包名 `moving-averages` 不变，无需 `@cap/*` 前缀或改名：

```typescript
// TypeScript / Node.js (npm: moving-averages)
import { ma, dma, ema, smma, wma } from "moving-averages";
const out = ema([1, 2, 3, 4, 5], 3);
```

```typescript
// CP（保持原包名 "moving-averages"）
import { ma, dma, ema, smma, wma } from "moving-averages";
let data: [5]f64 = [1.0, 2.0, 3.0, 4.0, 5.0];
let mut out: [5]f64 = undefined;
let n = ema(data[0..5], 3, out[0..5]);
```

## 安装

```bash
cap add moving-averages
```

## 快速上手与实战示例

### 1. 批量序列 API（完整对齐 npm `moving-averages`）

```typescript
import { ma, sma, dma, ema, smma, wma, cma, smaSlice, wmaSlice } from "moving-averages";

fn computeSeries(): void {
    let prices: [6]f64 = [10.0, 12.0, 11.0, 14.0, 15.0, 16.0];
    let mut out: [6]f64 = undefined;

    // 简单移动平均（sma 为 ma 别名）
    let n1 = ma(prices[0..6], 3, out[0..6]);
    let n2 = sma(prices[0..6], 3, out[0..6]);

    // 动态移动平均（自定义平滑因子 0 < alpha <= 1）
    let n3 = dma(prices[0..6], 0.5, out[0..6]);

    // 指数移动平均（alpha = 2 / (size + 1)）
    let n4 = ema(prices[0..6], 3, out[0..6]);

    // 平滑移动平均（Wilder SMMA，alpha = 1 / size）
    let n5 = smma(prices[0..6], 3, out[0..6]);

    // 线性加权移动平均 & 累积移动平均
    let n6 = wma(prices[0..6], 3, out[0..6]);
    let n7 = cma(prices[0..6], out[0..6]);

    // 单窗口切片聚合
    let s_avg = smaSlice(prices[0..6]);
    let w_avg = wmaSlice(prices[0..6]);
}
```

### 2. 零堆流式估计器（`Ema`、`Sma`、`Wma`、`Cma`）

```typescript
import { Ema, Sma, Wma, Cma } from "moving-averages";

fn streamingMetrics(): void {
    // 指数移动平均（按 alpha 或周期初始化）
    let mut ema_est = Ema.fromPeriod(5); // 或 Ema.new(0.2)
    let e1 = ema_est.update(100.0);
    let e2 = ema_est.update(105.0);
    ema_est.reset();

    // 环形缓冲简单移动平均（调用方提供栈上窗口切片）
    let mut sma_buf: [4]f64 = [0.0, 0.0, 0.0, 0.0];
    let mut sma_est = Sma.new();
    let s1 = sma_est.update(sma_buf[0..4], 10.0);
    let s2 = sma_est.update(sma_buf[0..4], 20.0);
    let cur = sma_est.average();

    // 环形缓冲加权移动平均
    let mut wma_buf: [4]f64 = [0.0, 0.0, 0.0, 0.0];
    let mut wma_est = Wma.new();
    let w1 = wma_est.update(wma_buf[0..4], 10.0);
    let w2 = wma_est.update(wma_buf[0..4], 20.0);

    // 累积移动平均
    let mut cma_est = Cma.new();
    let c1 = cma_est.update(10.0);
    let c2 = cma_est.update(20.0); // 15.0
    let c_avg = cma_est.average();
}
```

## API 速览

### 批量序列函数
- `ma(data: &[]f64, size: usize, out: []f64): usize` / `sma(...)`
- `dma(data: &[]f64, alpha: f64, out: []f64): usize`
- `ema(data: &[]f64, size: usize, out: []f64): usize`
- `smma(data: &[]f64, size: usize, out: []f64): usize`
- `wma(data: &[]f64, size: usize, out: []f64): usize`
- `cma(data: &[]f64, out: []f64): usize`
- `smaSlice(xs: []f64): f64`、`wmaSlice(xs: []f64): f64`、`maNan(): f64`

### 流式估计器类
- `Ema`：`new(alpha)` / `fromPeriod(period)` / `update(sample)` / `reset()`
- `Sma`：`new()` / `update(window, sample)` / `average()` / `reset()`
- `Wma`：`new()` / `update(window, sample)` / `reset()`
- `Cma`：`new()` / `update(sample)` / `average()` / `reset()`
