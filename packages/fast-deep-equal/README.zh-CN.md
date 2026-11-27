[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# fast-deep-equal

Zero-heap semantic JSON deep equality checker and root type classifier in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fast-deep-equal` ([https://github.com/epoberezkin/fast-deep-equal](https://github.com/epoberezkin/fast-deep-equal))

| 元信息 | 值 |
| --- | --- |
| 包名 | `fast-deep-equal` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add fast-deep-equal
```

```cp
import {
    ValueSpan,
    DecChar,
    isWs,
    skipWs,
    matchLit,
    hexDigit,
    invalidSpan,
    scanString,
} from "fast-deep-equal";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ValueSpan` | `class` | 字段: `start: usize, end: usize, kind: u8, valid: bool` |
| `DecChar` | `class` | 字段: `code: u32, next_pos: usize, done: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isWs` | `(ch: u8): bool` | 执行 `isWs` 条件判定，返回 `bool`。 |
| `skipWs` | `(s: string, pos: usize): usize` | 调用 `skipWs` 并返回 `usize`。 |
| `matchLit` | `(s: string, pos: usize, lit: string): bool` | 调用 `matchLit` 并返回 `bool`。 |
| `hexDigit` | `(ch: u8): i32` | 调用 `hexDigit` 并返回 `i32`。 |
| `invalidSpan` | `(): ValueSpan` | 调用 `invalidSpan` 并返回 `ValueSpan`。 |
| `scanString` | `(s: string, pos: usize): ValueSpan` | 调用 `scanString` 并返回 `ValueSpan`。 |
| `scanNumber` | `(s: string, pos: usize): ValueSpan` | 调用 `scanNumber` 并返回 `ValueSpan`。 |
| `parseValueSpan` | `(s: string, pos: usize, depth: u32): ValueSpan` | 通过 `parseValueSpan` 解析输入数据，返回 `ValueSpan`。 |
| `kindOfJson` | `(s: string): u8` | 调用 `kindOfJson` 并返回 `u8`。 |
| `isPlainObjectJson` | `(s: string): bool` | 执行 `isPlainObjectJson` 条件判定，返回 `bool`。 |
| `parseJsonNumberF64` | `(s: string): f64` | 通过 `parseJsonNumberF64` 解析输入数据，返回 `f64`。 |
| `nextDecodedChar` | `(s: string, pos: usize): DecChar` | 调用 `nextDecodedChar` 并返回 `DecChar`。 |
| `jsonStringsEqual` | `(a: string, b: string): bool` | 调用 `jsonStringsEqual` 并返回 `bool`。 |
| `countObjectMembers` | `(obj: string): usize` | 调用 `countObjectMembers` 并返回 `usize`。 |
| `arraysDeepEqual` | `(a: string, b: string, depth: u32): bool` | 调用 `arraysDeepEqual` 并返回 `bool`。 |
| `objectsDeepEqual` | `(a: string, b: string, depth: u32): bool` | 调用 `objectsDeepEqual` 并返回 `bool`。 |
| `valuesDeepEqual` | `(a: string, b: string, kind: u8, depth: u32): bool` | 调用 `valuesDeepEqual` 并返回 `bool`。 |
| `jsonDeepEqual` | `(a: string, b: string): bool` | 调用 `jsonDeepEqual` 并返回 `bool`。 |

## 最小调用示例

```cp
import { parseValueSpan } from "fast-deep-equal";

pub fn main() {
    let result = parseValueSpan("example", 0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
