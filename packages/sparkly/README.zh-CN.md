[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# sparkly

Zero-heap Unicode sparkline (▁▂▃▄▅▆▇█), horizontal bar chart, and summary statistics renderer for CLI & monitoring

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:sparkly` ([https://github.com/sindresorhus/sparkly](https://github.com/sindresorhus/sparkly))（原库名 `sparkly` → CP 包名 `sparkly`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `sparkly` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add sparkly
```

```cp
import {
    SeriesStats,
    SparkOptions,
    SparkResult,
    defaultOptions,
    computeStats,
    appendBlock,
    appendByte,
    appendBytes,
} from "sparkly";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SeriesStats` | `class` | 字段: `count: usize, minVal: f64, maxVal: f64, sum: f64, mean: f64, hasValues: bool` |
| `SparkOptions` | `class` | 字段: `useCustomBounds: bool, minBound: f64, maxBound: f64, asciiOnly: bool` |
| `SparkResult` | `class` | 字段: `buf: [512]u8, len: usize, truncated: bool` · 方法: `asSlice` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `defaultOptions` | `(): SparkOptions` | 调用 `defaultOptions` 并返回 `SparkOptions`。 |
| `computeStats` | `(values: &[]f64): SeriesStats` | 调用 `computeStats` 并返回 `SeriesStats`。 |
| `appendBlock` | `(res: &mut SparkResult, level: u8, asciiOnly: bool)` | 就地执行 `appendBlock` 操作。 |
| `appendByte` | `(res: &mut SparkResult, b: u8)` | 就地执行 `appendByte` 操作。 |
| `appendBytes` | `(res: &mut SparkResult, s: string)` | 就地执行 `appendBytes` 操作。 |
| `quantizeLevel` | `(value: f64, lo: f64, hi: f64): u8` | 调用 `quantizeLevel` 并返回 `u8`。 |
| `renderWithOptions` | `(values: &[]f64, opts: SparkOptions): SparkResult` | 调用 `renderWithOptions` 并返回 `SparkResult`。 |
| `render` | `(values: &[]f64): SparkResult` | 调用 `render` 并返回 `SparkResult`。 |
| `renderAscii` | `(values: &[]f64): SparkResult` | 调用 `renderAscii` 并返回 `SparkResult`。 |
| `renderBar` | `(ratio: f64, width: usize, fillAscii: bool): SparkResult` | 调用 `renderBar` 并返回 `SparkResult`。 |
| `renderDownsampled` | `(values: &[]f64, targetBuckets: usize, asciiOnly: bool): SparkResult` | 调用 `renderDownsampled` 并返回 `SparkResult`。 |
| `renderLabeledBar` | `(label: string, ratio: f64, barWidth: usize, fillAscii: bool): SparkResult` | 调用 `renderLabeledBar` 并返回 `SparkResult`。 |

## 最小调用示例

```cp
import { appendBytes } from "sparkly";

pub fn main() {
    appendBytes(/* res: &mut SparkResult */, "example");
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
