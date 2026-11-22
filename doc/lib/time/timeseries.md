# CP 标准库：时序数据存储、Rollup 降采样与异常检测手册 (`cap:timeseries`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:time`，导入请使用 `import { ... } from "cap:time"`。

## 一、 架构总览与设计准则

`cap:timeseries` 是 CP 语言官方内置的纯 CP **时序数据存储、时间对齐 Rollup 降采样、滑动平均与异常尖峰检测引擎**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，为系统遥测监控、Agent 性能观测与量化指标分析提供确定性内存安全的高性能原语。既可通过 `import { ... } from "cap:time"` 导入，也支持 `import cap from "cap"; cap.timeseries.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/timeseries/series.cp` | `// Source: cp/lib/timeseries/series.cp (...)` | `TimeSeriesError`、`AggOp`、`DataPoint`、`AnomalyPoint`、`SeriesSummary`、`TimeSeries` |
| `lib/timeseries/index.cp` | `// Source: cp/lib/timeseries/index.cp (...)` | `cap:timeseries` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误、枚举与数据结构

- **`TimeSeriesError`**：
  - `OutOfOrderTimestamp`：写入时间戳早于当前序列最后一个点（`ts_ms < last_ts`）。
  - `InvalidStep`：`downsample` 步长 `step_ms <= 0`。
  - `InvalidWindow`：`sma` 窗口长度 `window == 0`。
  - `EmptySeries`：对空序列执行 `summarize` 等要求非空的聚合操作。
  - `InvalidValue`：输入值为 `NaN`/`Inf`，或 `alpha`/`z_threshold` 参数超出有效范围。
  - `OutOfMemory`：底层缓冲区分配失败。
- **`AggOp`**：`Avg`、`Min`、`Max`、`Sum`、`Count`、`First`、`Last`、`P95`（提供 `toString()`）。
- **`DataPoint`**：`ts_ms: i64`、`value: f64`。
- **`AnomalyPoint`**：`ts_ms: i64`、`value: f64`、`z_score: f64`。
- **`SeriesSummary`**：`count: usize`、`min: f64`、`max: f64`、`sum: f64`、`mean: f64`、`stddev: f64`、`first: f64`、`last: f64`。

### 2. `TimeSeries` 核心方法

- **`TimeSeries.init(alloc, name, retention_ms): TimeSeries` / `deinit()`**：初始化与确定性释放。当 `retention_ms > 0` 时，每次 `push` 自动淘汰早于 `ts_ms - retention_ms` 的头部过期点（均摊 $O(1)$ 头部压缩）。
- **`push(ts_ms, val): void ! TimeSeriesError`**：校验有限数值与单调时间戳后追加数据点，并触发保留窗口淘汰。
- **`queryRange(alloc, start_ms, end_ms): []DataPoint ! TimeSeriesError`**：基于 $O(\log N)$ 二分查找定位闭区间 `[start_ms, end_ms]` 切片并返回拷贝（调用方用 `alloc.free` 释放）。
- **`downsample(alloc, start_ms, end_ms, step_ms, op): []DataPoint ! TimeSeriesError`**：按固定步长 `step_ms` 对齐分桶聚合非空桶，支持 `Avg` / `Min` / `Max` / `Sum` / `Count` / `First` / `Last` / `P95`。
- **`rate(alloc, start_ms, end_ms): []DataPoint ! TimeSeriesError`**：计算相邻单调递增计数器每秒变化率 `dv / dt_sec`，当检测到计数器归零重置（`v[i] < v[i - 1]`）时自动以 `0` 为新基线安全计算。
- **`sma(alloc, window): []DataPoint ! TimeSeriesError`**：$O(N)$ 滑动窗口简单移动平均（前 `window - 1` 个点按有效前缀长度计算）。
- **`ema(alloc, alpha): []DataPoint ! TimeSeriesError`**：指数加权移动平均（`alpha` 需位于 `(0, 1]`）。
- **`detectAnomaliesZScore(alloc, z_threshold): []AnomalyPoint ! TimeSeriesError`**：基于 Welford 均值与总体标准差计算 `z_score = (x - mean) / stddev`，返回 `|z_score| >= z_threshold` 的异常尖峰点。
- **`summarize(): SeriesSummary ! TimeSeriesError`**：单遍 Welford 算法计算全量统计摘要。
- **`toJson(): string ! TimeSeriesError`**：导出紧凑 JSON（调用方用 `_.alloc.free` 释放）。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { TimeSeries, AggOp } from "cap:time";
import cap from "cap";

pub fn demoTimeSeries(): ! {
    const alloc = std.heap.page_allocator;
    let ts = cap.timeseries.TimeSeries.init(alloc, "agent.latency_ms", 60000);
    defer ts.deinit();

    ts.push(1000, 12.0)?;
    ts.push(2000, 14.0)?;
    ts.push(3000, 13.0)?;
    ts.push(4000, 95.0)?;

    const summary = ts.summarize()?;
    const rolled = ts.downsample(alloc, 1000, 4000, 2000, AggOp.P95)?;
    defer alloc.free(rolled);

    const anomalies = ts.detectAnomaliesZScore(alloc, 1.5)?;
    defer alloc.free(anomalies);

    const json = ts.toJson()?;
    defer alloc.free(json);
    * = summary;
}
```
