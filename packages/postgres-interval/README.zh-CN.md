[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# postgres-interval

Zero-allocation PostgreSQL INTERVAL parser and ISO-8601 / PostgreSQL bidirectional formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:postgres-interval` ([https://github.com/bendrucker/postgres-interval](https://github.com/bendrucker/postgres-interval))

| 元信息 | 值 |
| --- | --- |
| 包名 | `postgres-interval` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add postgres-interval
```

```cp
import {
    PgInterval,
    PgivWriter,
    PgivIntRes,
    PgivFracRes,
    pgivInvalid,
    pgivZero,
    pgivSkipSpace,
    pgivParseUint,
} from "postgres-interval";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PgInterval` | `class` | 字段: `valid: bool, years: i32, months: i32, days: i32, hours: i32, minutes: i32, +2 项` |
| `PgivWriter` | `class` | 字段: `buf: []u8, pos: usize, overflow: bool` · 方法: `writeByte, putStr, putU64, writeI32, writeU64Pad2, writeMsTrimmed` |
| `PgivIntRes` | `class` | 字段: `ok: bool, val: i64, next: usize` |
| `PgivFracRes` | `class` | 字段: `ms: i32, next: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pgivInvalid` | `(): PgInterval` | 调用 `pgivInvalid` 并返回 `PgInterval`。 |
| `pgivZero` | `(): PgInterval` | 调用 `pgivZero` 并返回 `PgInterval`。 |
| `pgivSkipSpace` | `(s: string, start: usize): usize` | 调用 `pgivSkipSpace` 并返回 `usize`。 |
| `pgivParseUint` | `(s: string, start: usize): PgivIntRes` | 调用 `pgivParseUint` 并返回 `PgivIntRes`。 |
| `pgivParseFracMs` | `(s: string, start: usize): PgivFracRes` | 调用 `pgivParseFracMs` 并返回 `PgivFracRes`。 |
| `pgivAbsI32ToU64` | `(v: i32): u64` | 调用 `pgivAbsI32ToU64` 并返回 `u64`。 |
| `pgivParseIso` | `(s: string, start: usize, end: usize): PgInterval` | 调用 `pgivParseIso` 并返回 `PgInterval`。 |
| `parsePgInterval` | `(input: string): PgInterval` | 通过 `parsePgInterval` 解析输入数据，返回 `PgInterval`。 |
| `toIsoDuration` | `(iv: PgInterval, out: []u8): usize` | 调用 `toIsoDuration` 并返回 `usize`。 |
| `toPostgresString` | `(iv: PgInterval, out: []u8): usize` | 调用 `toPostgresString` 并返回 `usize`。 |
| `totalSecondsApprox` | `(iv: PgInterval): i64` | 调用 `totalSecondsApprox` 并返回 `i64`。 |

## 最小调用示例

```cp
import { parsePgInterval } from "postgres-interval";

pub fn main() {
    let result = parsePgInterval("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
