[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# iso8601

Zero-heap ISO-8601 calendar/week/ordinal date, time, timezone offset, datetime, and duration parser/formatter (crates.io iso8601 equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:iso8601` ([https://github.com/badboy/iso8601](https://github.com/badboy/iso8601))

| 元信息 | 值 |
| --- | --- |
| 包名 | `iso8601` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add iso8601
```

```cp
import {
    IsoDate,
    IsoTime,
    IsoDateTime,
    IsoDuration,
    isoIsDigit,
    isoParseDigits,
    isLeapYear,
    daysInMonth,
} from "iso8601";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `IsoDate` | `class` | 字段: `year: i32, month: u8, day: u8, ordinal: u16, week: u8, weekday: u8, +1 项` |
| `IsoTime` | `class` | 字段: `hour: u8, minute: u8, second: u8, millisecond: u16, tz_offset_minutes: i16, has_tz: bool` |
| `IsoDateTime` | `class` | 字段: `date: IsoDate, time: IsoTime` |
| `IsoDuration` | `class` | 字段: `years: u32, months: u32, weeks: u32, days: u32, hours: u32, minutes: u32, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isoIsDigit` | `(c: u8): bool` | 执行 `isoIsDigit` 条件判定，返回 `bool`。 |
| `isoParseDigits` | `(s: string, start: usize, count: usize): i64?` | 执行 `isoParseDigits` 条件判定，返回 `bool`。 |
| `isLeapYear` | `(year: i32): bool` | 执行 `isLeapYear` 条件判定，返回 `bool`。 |
| `daysInMonth` | `(year: i32, month: u8): u8` | 调用 `daysInMonth` 并返回 `u8`。 |
| `parseDate` | `(s: string): IsoDate?` | 通过 `parseDate` 解析输入数据，返回 `IsoDate?`。 |
| `parseTime` | `(s: string): IsoTime?` | 通过 `parseTime` 解析输入数据，返回 `IsoTime?`。 |
| `parseDateTime` | `(s: string): IsoDateTime?` | 通过 `parseDateTime` 解析输入数据，返回 `IsoDateTime?`。 |
| `parseDuration` | `(s: string): IsoDuration?` | 通过 `parseDuration` 解析输入数据，返回 `IsoDuration?`。 |
| `isoWrite2` | `(out: []u8, pos: usize, val: u8): usize` | 执行 `isoWrite2` 条件判定，返回 `bool`。 |
| `isoWrite4` | `(out: []u8, pos: usize, val: i32): usize` | 执行 `isoWrite4` 条件判定，返回 `bool`。 |
| `formatDateTime` | `(dt: IsoDateTime, out: []u8): usize` | 通过 `formatDateTime` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { isoParseDigits } from "iso8601";

pub fn main() {
    let result = isoParseDigits("example", 0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
