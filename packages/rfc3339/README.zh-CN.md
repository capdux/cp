[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# rfc3339

Zero-heap strict RFC 3339 timestamp parser, Unix epoch converter, and UTC formatter (reuses iso8601)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rfc3339` ([https://github.com/chronotope/chrono](https://github.com/chronotope/chrono))

| 元信息 | 值 |
| --- | --- |
| 包名 | `rfc3339` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add rfc3339
```

```cp
import {
    Rfc3339Timestamp,
    rfcIsDigit,
    daysFromCivil,
    civilFromDays,
    isValidRfc3339,
    parseRfc3339,
    fromUnixSeconds,
    formatRfc3339Utc,
} from "rfc3339";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Rfc3339Timestamp` | `class` | 字段: `dt: IsoDateTime, unix_seconds: i64, millisecond: u16` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `rfcIsDigit` | `(c: u8): bool` | 调用 `rfcIsDigit` 并返回 `bool`。 |
| `daysFromCivil` | `(year: i32, month: u8, day: u8): i64` | 调用 `daysFromCivil` 并返回 `i64`。 |
| `civilFromDays` | `(z_in: i64): IsoDate` | 调用 `civilFromDays` 并返回 `IsoDate`。 |
| `isValidRfc3339` | `(s: string): bool` | 执行 `isValidRfc3339` 条件判定，返回 `bool`。 |
| `parseRfc3339` | `(s: string): Rfc3339Timestamp?` | 通过 `parseRfc3339` 解析输入数据，返回 `Rfc3339Timestamp?`。 |
| `fromUnixSeconds` | `(unix_secs: i64): IsoDateTime` | 调用 `fromUnixSeconds` 并返回 `IsoDateTime`。 |
| `formatRfc3339Utc` | `(unix_secs: i64, out: []u8): usize` | 通过 `formatRfc3339Utc` 编码或格式化输出，返回 `usize`。 |
| `isRfcLeapYear` | `(year: i32): bool` | 执行 `isRfcLeapYear` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isValidRfc3339 } from "rfc3339";

pub fn main() {
    let result = isValidRfc3339("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
