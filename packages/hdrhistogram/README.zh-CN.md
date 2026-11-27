[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# hdrhistogram

CP 语言的高动态范围直方图（HDR Histogram）库，提供 RAII 原生句柄自动释放、协调遗漏（Coordinated Omission）校正、分位数/百分位数查询与多直方图合并能力。

- **源包对齐**：参考 Rust [`hdrhistogram`](https://github.com/HdrHistogram/HdrHistogram_rust)（`crates.io`，MIT）设计高层 API，底层绑定原生 C 引擎 `hdr_histogram.c`（`BSD-2-Clause`）。
- **包版本**：`1.0.0`

## 安装与导入

```bash
cap add hdrhistogram
```

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "hdrhistogram": "^1.0.0"
  }
}
```

```typescript
import { Histogram, HdrError } from "hdrhistogram";
```

## 核心 API 参考

### 错误集

| 符号 | 枚举项 | 说明 |
| :--- | :--- | :--- |
| `HdrError` | `InitFailed`, `InvalidHandle`, `ValueOutOfRange` | `Histogram` 各类可失败方法（`!T`）返回的错误类型 |

### `class Histogram`（RAII 高层封装）

字段：`handle: *hdr_histogram!?`、`lowest_trackable: i64`、`highest_trackable: i64`、`sigfigs: i32`。内置 `__cp_deinit` 析构方法，离开作用域时自动调用 `hdr_close`。

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `init` | `init(*_, lowest: i64, highest: i64, sigfigs: i32): !void` | 初始化底层直方图（`sigfigs` 范围 `1..5`） |
| `open` | `open(*_, lowest: i64, highest: i64, sigfigs: i32): !void` | 若已有旧句柄则先关闭，再重新初始化 |
| `record` | `record(value: i64): !void` | 记录单个样本；越界返回 `HdrError.ValueOutOfRange` |
| `recordCount` / `recordN` | `recordCount(value: i64, count: i64): !void` | 批量记录 `count` 次相同样本 `value` |
| `recordCorrected` / `recordCorrecting` | `recordCorrected(value: i64, expected_interval: i64): !void` | 带协调遗漏校正的样本记录 |
| `recordCorrectingN` | `recordCorrectingN(value: i64, count: i64, expected_interval: i64): !void` | 带协调遗漏校正的批量记录 |
| `saturatingRecord` | `saturatingRecord(value: i64): !void` | 将 `value` 钳制到 `[lowest_trackable, highest_trackable]` 后记录 |
| `reset` | `reset(): !void` | 清零所有计数 |
| `memorySize` | `memorySize(): !usize` | 返回底层 C 直方图占用的字节数 |
| `min` / `max` | `min(): !i64` / `max(): !i64` | 返回已记录样本的最小值 / 最大值 |
| `mean` | `mean(): !f64` | 返回算术平均值 |
| `stddev` / `stdev` | `stddev(): !f64` | 返回标准差 |
| `percentile` / `valueAtPercentile` | `percentile(p: f64): !i64` | 返回第 `p` 百分位值（`0.0..=100.0`） |
| `valueAtQuantile` | `valueAtQuantile(q: f64): !i64` | 返回第 `q` 分位值（`0.0..=1.0`） |
| `len` / `isEmpty` | `len(): !u64` / `isEmpty(): !bool` | 返回总样本数 / 判断是否为空 |
| `percentileBelow` / `quantileBelow` | `percentileBelow(value: i64): !f64` / `quantileBelow(value: i64): !f64` | 返回小于等于 `value` 的样本占比（百分数 `0..100` 或分位数 `0..1`） |
| `add` | `add(other: Histogram): !i64` | 合并另一个直方图，返回被丢弃的越界计数 |
| `addCorrecting` | `addCorrecting(other: Histogram, expected_interval: i64): !i64` | 带协调遗漏校正的直方图合并 |
| `countAt` | `countAt(value: i64): !i64` | 查询 `value` 所在精度桶的计数 |
| `equivalent` | `equivalent(a: i64, b: i64): !bool` | 判断 `a` 与 `b` 是否落在同一等价桶 |
| `lowestEquivalent` / `medianEquivalent` | `lowestEquivalent(value: i64): !i64` / `medianEquivalent(value: i64): !i64` | 查询 `value` 所在等价区间的下界 / 中位数 |
| `nextNonEquivalent` / `sizeOfEquivalentRange` | `nextNonEquivalent(value: i64): !i64` / `sizeOfEquivalentRange(value: i64): !i64` | 查询下一个非等价桶起点 / 当前桶宽度 |

### 底层 C FFI 与辅助函数

同时导出原始 C 接口供底层互操作：`hdr_histogram`、`hdr_init`、`hdr_close`、`hdr_reset`、`hdr_get_memory_size`、`hdr_record_value`、`hdr_record_values`、`hdr_record_corrected_value`、`hdr_record_corrected_values`、`hdr_add`、`hdr_add_while_correcting_for_coordinated_omission`、`hdr_min`、`hdr_max`、`hdr_mean`、`hdr_stddev`、`hdr_value_at_percentile`、`hdr_count_at_value`、`hdr_values_are_equivalent`、`hdr_lowest_equivalent_value`、`hdr_median_equivalent_value`、`hdr_next_non_equivalent_value`、`hdr_size_of_equivalent_value_range`、`hdrClampF64`、`hdrTotalCount`。

## 使用示例

### 1. 记录延迟样本并查询分位数

```typescript
import { Histogram } from "hdrhistogram";
import { print } from "cap:io";

fn main(): !void {
    let mut h = Histogram { handle: null, lowest_trackable: 0, highest_trackable: 0, sigfigs: 0 };
    h.open(1, 3_600_000_000, 3)?;

    h.record(120)?;
    h.record(450)?;
    h.recordN(1_250, 5)?;
    h.saturatingRecord(9_999_999_999)?;

    let p50 = h.valueAtPercentile(50.0)?;
    let p99 = h.valueAtQuantile(0.99)?;
    let total = h.len()?;
    print("total={} p50={} p99={} mean={}\n", total, p50, p99, h.mean()?);
}
```

### 2. 协调遗漏校正与跨工作线程合并

```typescript
import { Histogram } from "hdrhistogram";

fn mergeWorkerHistograms(): !i64 {
    let mut a = Histogram { handle: null, lowest_trackable: 0, highest_trackable: 0, sigfigs: 0 };
    let mut b = Histogram { handle: null, lowest_trackable: 0, highest_trackable: 0, sigfigs: 0 };
    a.open(1, 1_000_000, 3)?;
    b.open(1, 1_000_000, 3)?;

    a.recordCorrected(5_000, 1_000)?;
    b.recordCorrectingN(2_500, 4, 1_000)?;

    let dropped = a.add(b)?;
    return dropped;
}
```

## 与源包差异与 CP 安全亮点

- **确定性 RAII 释放**：`Histogram.__cp_deinit` 在作用域结束时自动调用 `hdr_close`，无需手动清理 C 堆内存，杜绝泄漏。
- **显式错误联合（`!T`）**：所有可能失败的操作统一返回 `HdrError`（`InitFailed` / `InvalidHandle` / `ValueOutOfRange`），通过 `?` 零开销传播错误。
- **对齐 Rust 习惯接口**：完整补齐 `saturatingRecord`、`valueAtQuantile`、`quantileBelow`、`stdev`、`recordN` 等高频方法，兼容原生 C 引擎的高性能。
