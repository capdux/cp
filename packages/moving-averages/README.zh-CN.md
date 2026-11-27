[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# moving-averages

纯 CP 实现的零堆流式与批处理移动平均库，完整覆盖 `MA`/`SMA`、`EMA`、`DMA`、`SMMA`、`WMA` 与 `CMA`。

- **源包对齐**：融合 npm [`moving-averages`](https://github.com/kaelzhang/node-moving-averages)（`MIT`）的批处理序列函数与 Rust [`simple_moving_average`](https://github.com/emilec/simple_moving_average)（`crates.io`，`MIT`）的流式计算器。
- **包版本**：`1.0.0`

## 安装与导入

```bash
cap add moving-averages
```

在 `cap.json` 中声明依赖：

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

## 核心 API 参考

### 流式计算器（零堆）

| 类型 | 方法 | 说明 |
| :--- | :--- | :--- |
| `Ema` | `Ema.new(alpha: f64)`、`Ema.fromPeriod(period: u32)`、`update(*_, sample: f64): f64`、`reset(*_)` | 指数移动平均（`alpha = 2 / (period + 1)`） |
| `Sma` | `Sma.new()`、`update(*_, window: []f64, sample: f64): f64`、`average(): f64`、`reset(*_)` | 基于调用方环形窗口 `window` 的 O(1) 简单移动平均 |
| `Cma` | `Cma.new()`、`update(*_, sample: f64): f64`、`average(): f64`、`reset(*_)` | 累积移动平均 |
| `Wma` | `Wma.new()`、`update(*_, window: []f64, sample: f64): f64`、`reset(*_)` | 基于调用方环形窗口 `window` 的线性加权移动平均 |

### 批处理序列与单次切片函数

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `ma` / `sma` | `ma(data: &[]f64, size: usize, out: []f64): usize` | 简单移动平均序列（前 `size - 1` 位填充 `maNan()`） |
| `dma` | `dma(data: &[]f64, alpha: f64, out: []f64): usize` | 指定平滑系数 `alpha` 的动态移动平均序列 |
| `ema` | `ema(data: &[]f64, size: usize, out: []f64): usize` | 指数移动平均序列（`alpha = 2 / (size + 1)`） |
| `smma` | `smma(data: &[]f64, size: usize, out: []f64): usize` | 平滑（Wilder）移动平均序列（`alpha = 1 / size`） |
| `wma` | `wma(data: &[]f64, size: usize, out: []f64): usize` | 线性加权移动平均序列 |
| `cma` | `cma(data: &[]f64, out: []f64): usize` | 累积移动平均序列 |
| `smaSlice` / `wmaSlice` | `smaSlice(xs: []f64): f64` / `wmaSlice(xs: []f64): f64` | 计算整个切片的单次标量 SMA / WMA |
| `maNan` | `maNan(): f64` | 返回用于预热窗口占位的 IEEE-754 `NaN` |

## 使用示例

### 1. 实时指标流式 `Sma` 与 `Ema`

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

### 2. 批处理 `ema` 与 `wma` 序列计算

```typescript
import { ema, wma } from "moving-averages";

fn computeIndicators(prices: &[]f64, ema_out: []f64, wma_out: []f64): usize {
    let n1 = ema(prices, 3, ema_out);
    let n2 = wma(prices, 3, wma_out);
    return if (n1 < n2) { n1 } else { n2 };
}
```

## 与源包差异与 CP 安全亮点

- **调用方持有环形窗口**：流式 `Sma` 与 `Wma` 接受任意长度的 `window: []f64` 栈数组，彻底免除堆分配。
- **零分配批处理接口**：所有批处理函数接收只读 `&[]f64` 并直写 `out: []f64`，返回实际写入长度。
- **流式与批处理兼备**：既保留 npm `moving-averages` 的预热期 `NaN`（`maNan()`）语义，又提供低延迟在线更新类。
