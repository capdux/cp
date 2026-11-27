[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# server-timing

Zero-allocation W3C Server-Timing HTTP response header parser and formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:server-timing` ([https://github.com/yosuke-furukawa/server-timing](https://github.com/yosuke-furukawa/server-timing))

| 元信息 | 值 |
| --- | --- |
| 包名 | `server-timing` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add server-timing
```

```cp
import {
    TimingMetric,
    ParsedFloat,
    parseFloatSimple,
    parseSingleMetric,
    parseHeader,
    findMetric,
    totalDurationMs,
    isTokenChar,
} from "server-timing";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TimingMetric` | `class` | 字段: `name: string, has_dur: bool, dur_ms: f64, desc: string` |
| `ParsedFloat` | `class` | 字段: `val: f64, ok: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `parseFloatSimple` | `(s: string): ParsedFloat` | 通过 `parseFloatSimple` 解析输入数据，返回 `ParsedFloat`。 |
| `parseSingleMetric` | `(seg: string, out: &mut TimingMetric): bool` | 通过 `parseSingleMetric` 解析输入数据，返回 `bool`。 |
| `parseHeader` | `(header: string, out: []TimingMetric): usize` | 通过 `parseHeader` 解析输入数据，返回 `usize`。 |
| `findMetric` | `(header: string, name: string, out: &mut TimingMetric): bool` | 查询或读取目标数据，返回 `bool`。 |
| `totalDurationMs` | `(header: string): f64` | 调用 `totalDurationMs` 并返回 `f64`。 |
| `isTokenChar` | `(b: u8): bool` | 执行 `isTokenChar` 条件判定，返回 `bool`。 |
| `descNeedsQuotes` | `(s: string): bool` | 调用 `descNeedsQuotes` 并返回 `bool`。 |
| `pushSlice` | `(dst: []u8, pos: usize, src: string): i64` | 写入或更新状态数据。 |
| `pushU64` | `(dst: []u8, pos: usize, val: u64): i64` | 写入或更新状态数据。 |
| `pushDurMs` | `(dst: []u8, pos: usize, dur_ms: f64): i64` | 写入或更新状态数据。 |
| `formatMetric` | `(metric: TimingMetric, out: []u8): usize` | 通过 `formatMetric` 编码或格式化输出，返回 `usize`。 |
| `formatHeader` | `(metrics: &[]TimingMetric, out: []u8): usize` | 通过 `formatHeader` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseFloatSimple } from "server-timing";

pub fn main() {
    let result = parseFloatSimple("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
