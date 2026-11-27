[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# strftime

Zero-heap POSIX strftime & ISO-8601 date/time formatter for Unix timestamps and broken-down calendar dates

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:strftime` ([https://github.com/samsonjs/strftime](https://github.com/samsonjs/strftime))

| 元信息 | 值 |
| --- | --- |
| 包名 | `strftime` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add strftime
```

```cp
import {
    DateTime,
    FmtWriter,
    isLeapYear,
    daysInMonth,
    dayOfYear,
    fromUnixSeconds,
    weekdayShort,
    weekdayFull,
} from "strftime";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DateTime` | `class` | 字段: `year: i32, month: u8, day: u8, hour: u8, minute: u8, second: u8, +3 项` |
| `FmtWriter` | `class` | 字段: `out: []u8, pos: usize` · 方法: `putByte, putStr, putPad2, putPad3, putPad4` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isLeapYear` | `(y: i32): bool` | 执行 `isLeapYear` 条件判定，返回 `bool`。 |
| `daysInMonth` | `(y: i32, m: u8): u8` | 调用 `daysInMonth` 并返回 `u8`。 |
| `dayOfYear` | `(y: i32, m: u8, d: u8): u16` | 调用 `dayOfYear` 并返回 `u16`。 |
| `fromUnixSeconds` | `(unix_sec: i64, tz_offset_sec: i32): DateTime` | 调用 `fromUnixSeconds` 并返回 `DateTime`。 |
| `weekdayShort` | `(w: u8): string` | 调用 `weekdayShort` 并返回 `string`。 |
| `weekdayFull` | `(w: u8): string` | 调用 `weekdayFull` 并返回 `string`。 |
| `monthShort` | `(m: u8): string` | 调用 `monthShort` 并返回 `string`。 |
| `monthFull` | `(m: u8): string` | 调用 `monthFull` 并返回 `string`。 |
| `strftime` | `(fmt: string, dt: DateTime, out: []u8): string` | 调用 `strftime` 并返回 `string`。 |
| `formatUnix` | `(fmt: string, unix_sec: i64, tz_offset_sec: i32, out: []u8): string` | 通过 `formatUnix` 编码或格式化输出，返回 `string`。 |
| `formatIso8601Utc` | `(unix_sec: i64, out: []u8): string` | 通过 `formatIso8601Utc` 编码或格式化输出，返回 `string`。 |

## 最小调用示例

```cp
import { formatUnix } from "strftime";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatUnix("example", 0, 0, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
