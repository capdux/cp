[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# postgres-date

Zero-allocation PostgreSQL TIMESTAMP / TIMESTAMPTZ / DATE / infinity parser with microsecond and timezone offset precision (clean-room MIT implementation of npm postgres-date, #505 / 112M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:postgres-date` ([https://github.com/bendrucker/postgres-date](https://github.com/bendrucker/postgres-date)) | 许可：`MIT`

| 元信息 | 值 |
| --- | --- |
| 包名 | `postgres-date` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add postgres-date
```

```cp
import {
    PgDateKind,
    PgDateTime,
    pgdParseDigits,
    pgdIsLeap,
    pgdDaysInMonth,
    parsePgDate,
    toUnixMs,
} from "postgres-date";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `PgDateKind` | `Invalid, Finite, PosInfinity, NegInfinity` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PgDateTime` | `class` | 字段: `kind: PgDateKind, year: i32, month: u8, day: u8, hour: u8, minute: u8, +4 项` · 方法: `isValid` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pgdParseDigits` | `(s: string): i32?` | 调用 `pgdParseDigits` 并返回 `i32?`。 |
| `pgdIsLeap` | `(y: i32): bool` | 调用 `pgdIsLeap` 并返回 `bool`。 |
| `pgdDaysInMonth` | `(y: i32, m: u8): u8` | 调用 `pgdDaysInMonth` 并返回 `u8`。 |
| `parsePgDate` | `(input: string): PgDateTime` | 通过 `parsePgDate` 解析输入数据，返回 `PgDateTime`。 |
| `toUnixMs` | `(dt: PgDateTime): i64?` | 调用 `toUnixMs` 并返回 `i64?`。 |

## 最小调用示例

```cp
import { parsePgDate } from "postgres-date";

pub fn main() {
    let result = parsePgDate("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
