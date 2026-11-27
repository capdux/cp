[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# winnow

Zero-allocation byte-level zero-copy parser combinators (winnow / nom parity) with rollback-on-failure cursor semantics for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:winnow` ([https://github.com/winnow-rs/winnow](https://github.com/winnow-rs/winnow))（原库名 `winnow` → CP 包名 `winnow`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `winnow` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add winnow
```

```cp
import {
    Cursor,
    SpanResult,
    IntResult,
    UintResult,
    nbFailSpan,
    tag,
    tagNoCase,
    charLit,
} from "winnow";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Cursor` | `class` | 字段: `data: string, pos: usize` · 方法: `new, remaining, isEmpty, peek, rest, sliceFrom` |
| `SpanResult` | `class` | 字段: `ok: bool, start: usize, len: usize` |
| `IntResult` | `class` | 字段: `ok: bool, value: i64` |
| `UintResult` | `class` | 字段: `ok: bool, value: u64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `nbFailSpan` | `(start: usize): SpanResult` | 调用 `nbFailSpan` 并返回 `SpanResult`。 |
| `tag` | `(cur: &mut Cursor, lit: string): bool` | 调用 `tag` 并返回 `bool`。 |
| `tagNoCase` | `(cur: &mut Cursor, lit: string): bool` | 调用 `tagNoCase` 并返回 `bool`。 |
| `charLit` | `(cur: &mut Cursor, ch: u8): bool` | 调用 `charLit` 并返回 `bool`。 |
| `take` | `(cur: &mut Cursor, count: usize): SpanResult` | 调用 `take` 并返回 `SpanResult`。 |
| `takeUntil` | `(cur: &mut Cursor, needle: string): SpanResult` | 调用 `takeUntil` 并返回 `SpanResult`。 |
| `takeTillByte` | `(cur: &mut Cursor, stop: u8): SpanResult` | 调用 `takeTillByte` 并返回 `SpanResult`。 |
| `space0` | `(cur: &mut Cursor): SpanResult` | 调用 `space0` 并返回 `SpanResult`。 |
| `multispace0` | `(cur: &mut Cursor): SpanResult` | 调用 `multispace0` 并返回 `SpanResult`。 |
| `digit1` | `(cur: &mut Cursor): SpanResult` | 调用 `digit1` 并返回 `SpanResult`。 |
| `alpha1` | `(cur: &mut Cursor): SpanResult` | 调用 `alpha1` 并返回 `SpanResult`。 |
| `alphanumeric1` | `(cur: &mut Cursor): SpanResult` | 调用 `alphanumeric1` 并返回 `SpanResult`。 |
| `hexDigit1` | `(cur: &mut Cursor): SpanResult` | 调用 `hexDigit1` 并返回 `SpanResult`。 |
| `parseU64` | `(cur: &mut Cursor): UintResult` | 通过 `parseU64` 解析输入数据，返回 `UintResult`。 |
| `parseI64` | `(cur: &mut Cursor): IntResult` | 通过 `parseI64` 解析输入数据，返回 `IntResult`。 |
| `delimited` | `(cur: &mut Cursor, open: u8, close: u8): SpanResult` | 调用 `delimited` 并返回 `SpanResult`。 |

## 最小调用示例

```cp
import { tag } from "winnow";

pub fn main() {
    let result = tag(/* cur: &mut Cursor */, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
