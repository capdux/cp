[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# hot-shots

Zero-heap StatsD and DogStatsD UDP metric packet encoder/decoder supporting Counter, Timing, Gauge, Set, sample rates, and tags.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:hot-shots` ([https://github.com/brightcove/hot-shots](https://github.com/brightcove/hot-shots))（原库名 `hot-shots` → CP 包名 `hot-shots`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `hot-shots` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add hot-shots
```

```cp
import {
    StatsdMetric,
    isValidMetricName,
    isValidTags,
    appendByte,
    appendSlice,
    appendI64,
    formatMetric,
    parseSampleRateMilli,
} from "hot-shots";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `StatsdMetric` | `class` | 字段: `name_start: usize, name_len: usize, value: i64, kind: u8, sample_rate_milli: u16, tags_start: usize, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isValidMetricName` | `(name: &[]u8): bool` | 执行 `isValidMetricName` 条件判定，返回 `bool`。 |
| `isValidTags` | `(tags: &[]u8): bool` | 执行 `isValidTags` 条件判定，返回 `bool`。 |
| `appendByte` | `(dst: []u8, pos: usize, b: u8): i64` | 调用 `appendByte` 并返回 `i64`。 |
| `appendSlice` | `(dst: []u8, pos: usize, s: &[]u8): i64` | 调用 `appendSlice` 并返回 `i64`。 |
| `appendI64` | `(dst: []u8, pos: usize, v: i64): i64` | 调用 `appendI64` 并返回 `i64`。 |
| `formatMetric` | `(name: &[]u8, value: i64, kind: u8, sample_rate_milli: u16, tags: &[]u8, dst: []u8): i64` | 通过 `formatMetric` 编码或格式化输出，返回 `i64`。 |
| `parseSampleRateMilli` | `(s: &[]u8): i32` | 通过 `parseSampleRateMilli` 解析输入数据，返回 `i32`。 |
| `parseMetric` | `(packet: &[]u8): StatsdMetric` | 通过 `parseMetric` 解析输入数据，返回 `StatsdMetric`。 |

## 最小调用示例

```cp
import { isValidMetricName } from "hot-shots";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isValidMetricName(out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
