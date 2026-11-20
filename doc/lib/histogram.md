# CP 标准库：高动态范围延迟直方图与分位统计手册 (`cap:histogram`)

## 一、 架构总览与设计准则

`cap:histogram` 是 CP 语言官方内置的纯 CP **工业级高动态范围延迟直方图（HDR Histogram）与流式速率统计库**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**。基于 Gil Tene 对数-线性子分桶（Log-Linear Sub-bucketing）算法，支持 1..5 位有效数字精度，以极低内存占用覆盖微秒到小时级动态范围，并提供 Coordinated Omission 修正、多核线程本地直方图无损聚合、EWMA 速率平滑、固定环形槽位滑动窗口计数以及 Prometheus 指标导出。支持 `import { ... } from "cap:histogram"` 与 `import cap from "cap"; cap.histogram.*` 动态挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/histogram/hdr.cp` | `// Source: cp/lib/histogram/hdr.cp (...)` | `HistogramError`、`HdrHistogram`、`RecordedBucket`、`RecordedIter` |
| `lib/histogram/ewma.cp` | `// Source: cp/lib/histogram/ewma.cp (...)` | `Ewma`（1m/5m/15m 及自定义 alpha）与 `SlidingWindowCounter` |
| `lib/histogram/export.cp` | `// Source: cp/lib/histogram/export.cp (...)` | `formatSummary` 与 `formatPrometheusBuckets` 导出辅助函数 |
| `lib/histogram/index.cp` | `// Source: cp/lib/histogram/index.cp (...)` | `cap:histogram` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集 (`HistogramError`)

- **`InvalidPrecision`**：`significant_digits` 不在 `[1, 5]` 范围内，或 `Ewma` 的 `alpha` 不在 `(0.0, 1.0]` 内。
- **`InvalidRange`**：`lowest_discernible_value < 1` 或 `highest_trackable_value < 2 * lowest_discernible_value` 或位宽溢出。
- **`ValueOutOfRange`**：`recordValue` / `recordValues` 记录的数值超过 `highest_trackable_value`。
- **`IncompatibleHistogram`**：`merge` 时两个 `HdrHistogram` 的范围或有效数字精度不一致。
- **`InvalidWindow`**：`SlidingWindowCounter` 的窗口时长或桶数量非法，或 `Ewma` 的 `interval_secs <= 0`。
- **`OutOfMemory`**：底层分桶数组或格式化缓冲区内存分配失败。

### 2. 高动态范围直方图 (`HdrHistogram`)

- **`init(alloc, lowest_discernible_value, highest_trackable_value, significant_digits): HdrHistogram ! HistogramError`**：初始化覆盖 `[lowest, highest]` 且保证 `significant_digits`（`1..5`）有效数字精度的对数子桶直方图。
- **`deinit(*_)`**：释放底层计数数组。
- **`recordValue(*_, value: u64): void ! HistogramError`**：记录单次采样。
- **`recordValues(*_, value: u64, count: u64): void ! HistogramError`**：批量记录 `count` 次采样。
- **`recordValueWithExpectedInterval(*_, value: u64, expected_interval: u64): void ! HistogramError`**：记录采样并执行 Coordinated Omission 修正（当 `value > expected_interval > 0` 时自动回填停顿期间缺失的采样点）。
- **`reset(*_)`**：清零所有计数并重置 `min/max/totalCount`，保留已分配内存。
- **`merge(*_, other: HdrHistogram): void ! HistogramError`**：无损聚合相同配置的另一直方图实例。
- **`minValue(): u64`** / **`maxValue(): u64`** / **`totalCount(): u64`**：O(1) 获取最小、最大与总样本数。
- **`mean(): f64`** / **`stddev(): f64`**：计算均值与总体标准差。
- **`valueAtPercentile(p: f64): u64`**：查询任意分位数（支持 P50/P95/P99/P99.9/P99.99）。
- **`percentileAtOrBelow(value: u64): f64`**：查询小于等于 `value` 的样本百分比。
- **`iterRecorded(): RecordedIter`**：返回升序遍历所有非零桶的零分配迭代器。
- **`formatSummary(alloc): []u8 ! HistogramError`**：导出包含 count/min/max/mean/stddev 与 P50/P95/P99/P99.9/P99.99 的可读摘要。
- **`formatPrometheusBuckets(alloc, metric_name, le_bounds): []u8 ! HistogramError`**：导出 Prometheus 直方图文本格式（`_bucket{le=...}`、`+Inf`、`_sum`、`_count`）。

### 3. 指数加权移动平均 (`Ewma`) 与滑动窗口 (`SlidingWindowCounter`)

- **`Ewma`**：
  - `init(alpha, interval_secs)` / `initOneMinute()` / `initFiveMinutes()` / `initFifteenMinutes()`
  - `update(*_, n: u64)`、`tick(*_)`、`rate(): f64`
- **`SlidingWindowCounter`**：
  - `init(alloc, window_ms, bucket_count)`、`deinit(*_)`、`reset(*_)`
  - `record(*_, now_ms: i64, count: u64)`、`sum(*_, now_ms: i64): u64`、`ratePerSec(*_, now_ms: i64): f64`

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { HdrHistogram, Ewma, SlidingWindowCounter } from "cap:histogram";

pub fn demoHistogram(): ! {
    const alloc = std.testing.allocator;
    let hist = HdrHistogram.init(alloc, 1, 3_600_000_000, 3)?;
    defer hist.deinit();

    hist.recordValue(120)?;
    hist.recordValue(450)?;
    hist.recordValueWithExpectedInterval(50_000, 10_000)?;

    const p99 = hist.valueAtPercentile(99.0);
    * = p99;

    const summary = hist.formatSummary(alloc)?;
    defer alloc.free(summary);
}
```
