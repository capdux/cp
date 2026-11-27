[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# tzfile

Zero-heap RFC 8536 TZif binary header parser, POSIX TZ string parser, and UTC offset transition evaluator (crates.io tzfile equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tzfile` ([https://github.com/x-hgg-x/tz-rs](https://github.com/x-hgg-x/tz-rs))

| 元信息 | 值 |
| --- | --- |
| 包名 | `tzfile` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add tzfile
```

```cp
import {
    TzifHeader,
    PosixTz,
    TzTransition,
    TzAbbrRes,
    TzOffRes,
    tzReadU32Be,
    isTzifMagic,
    parseTzifHeader,
} from "tzfile";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TzifHeader` | `class` | 字段: `version: u8, isutcnt: u32, isstdcnt: u32, leapcnt: u32, timecnt: u32, typecnt: u32, +1 项` |
| `PosixTz` | `class` | 字段: `std_abbr: string, std_offset_secs: i32, has_dst: bool, dst_abbr: string, dst_offset_secs: i32` |
| `TzTransition` | `class` | 字段: `at_unix: i64, offset_secs: i32, is_dst: bool` |
| `TzAbbrRes` | `class` | 字段: `abbr: string, next_pos: usize, ok: bool` |
| `TzOffRes` | `class` | 字段: `secs: i32, next_pos: usize, ok: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `tzReadU32Be` | `(buf: []u8, off: usize): u32` | 调用 `tzReadU32Be` 并返回 `u32`。 |
| `isTzifMagic` | `(buf: []u8): bool` | 执行 `isTzifMagic` 条件判定，返回 `bool`。 |
| `parseTzifHeader` | `(buf: []u8): TzifHeader?` | 通过 `parseTzifHeader` 解析输入数据，返回 `TzifHeader?`。 |
| `tzIsAlpha` | `(c: u8): bool` | 调用 `tzIsAlpha` 并返回 `bool`。 |
| `tzIsDigit` | `(c: u8): bool` | 调用 `tzIsDigit` 并返回 `bool`。 |
| `tzParseAbbr` | `(s: string, pos_in: usize): TzAbbrRes` | 调用 `tzParseAbbr` 并返回 `TzAbbrRes`。 |
| `tzParsePosixOffset` | `(s: string, pos_in: usize): TzOffRes` | 调用 `tzParsePosixOffset` 并返回 `TzOffRes`。 |
| `parsePosixTz` | `(spec: string): PosixTz?` | 通过 `parsePosixTz` 解析输入数据，返回 `PosixTz?`。 |
| `lookupTransitionOffset` | `(transitions: []TzTransition, unix_secs: i64, default_offset: i32): i32` | 查询或读取目标数据，返回 `i32`。 |
| `utcToLocalSeconds` | `(unix_secs: i64, offset_secs: i32): i64` | 调用 `utcToLocalSeconds` 并返回 `i64`。 |

## 最小调用示例

```cp
import { parsePosixTz } from "tzfile";

pub fn main() {
    let result = parsePosixTz("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
