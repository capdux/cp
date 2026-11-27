[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# d3-time-format

Zero-heap UTC strftime/strptime date formatter and parser for CP (clean-room ISC implementation of npm d3-time-format).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:d3-time-format` ([https://github.com/d3/d3-time-format](https://github.com/d3/d3-time-format))（原库名 `d3-time-format` → CP 包名 `d3-time-format`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `d3-time-format` |
| 版本 | `1.0.0` |
| 许可证 | `ISC` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add d3-time-format
```

```cp
import {
    D3tfDateTime,
    D3tfUintParse,
    d3tfIsLeapYear,
    d3tfDaysInMonth,
    d3tfDayOfYear,
    d3tfFromUnixMs,
    d3tfToUnixMs,
    d3tfMonthShort,
} from "d3-time-format";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `D3tfDateTime` | `class` | 字段: `year: u64, month: u64, day: u64, hour: u64, minute: u64, second: u64, +3 项` |
| `D3tfUintParse` | `class` | 字段: `ok: bool, val: u64, next_pos: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `d3tfIsLeapYear` | `(y: u64): bool` | 调用 `d3tfIsLeapYear` 并返回 `bool`。 |
| `d3tfDaysInMonth` | `(y: u64, m: u64): u64` | 调用 `d3tfDaysInMonth` 并返回 `u64`。 |
| `d3tfDayOfYear` | `(y: u64, m: u64, d: u64): u64` | 调用 `d3tfDayOfYear` 并返回 `u64`。 |
| `d3tfFromUnixMs` | `(unix_ms: i64): D3tfDateTime` | 调用 `d3tfFromUnixMs` 并返回 `D3tfDateTime`。 |
| `d3tfToUnixMs` | `(year: u64, month: u64, day: u64, hour: u64, minute: u64, second: u64, ms: u64): i64` | 调用 `d3tfToUnixMs` 并返回 `i64`。 |
| `d3tfMonthShort` | `(m: u64): string` | 调用 `d3tfMonthShort` 并返回 `string`。 |
| `d3tfMonthFull` | `(m: u64): string` | 调用 `d3tfMonthFull` 并返回 `string`。 |
| `d3tfWeekdayShort` | `(w: u64): string` | 调用 `d3tfWeekdayShort` 并返回 `string`。 |
| `d3tfWeekdayFull` | `(w: u64): string` | 调用 `d3tfWeekdayFull` 并返回 `string`。 |
| `d3tfWritePadded` | `(out: []u8, pos: usize, val: u64, width: usize): usize` | 调用 `d3tfWritePadded` 并返回 `usize`。 |
| `utcFormat` | `(spec: string, unix_ms: i64, out: []u8): usize` | 调用 `utcFormat` 并返回 `usize`。 |
| `d3tfMatchWordCI` | `(input: string, pos: usize, word: string): bool` | 调用 `d3tfMatchWordCI` 并返回 `bool`。 |
| `d3tfParseDigits` | `(input: string, pos: usize, min_d: usize, max_d: usize): D3tfUintParse` | 调用 `d3tfParseDigits` 并返回 `D3tfUintParse`。 |
| `utcParse` | `(spec: string, input: string): i64?` | 调用 `utcParse` 并返回 `i64?`。 |

## 最小调用示例

```cp
import { utcFormat } from "d3-time-format";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = utcFormat("example", 0, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
