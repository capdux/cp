[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# tinystr

Zero-allocation stack-inline fixed-capacity ASCII strings (TinyAsciiStr4, TinyAsciiStr8, TinyAsciiStr16) with u32/u64 packing and BCP-47 case transforms (ICU4X tinystr parity) in pure CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tinystr` ([https://github.com/unicode-org/icu4x](https://github.com/unicode-org/icu4x))

| 元信息 | 值 |
| --- | --- |
| 包名 | `tinystr` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add tinystr
```

```cp
import {
    TinyAsciiStr4,
    TinyAsciiStr4Result,
    TinyAsciiStr8,
    TinyAsciiStr8Result,
    TinyAsciiStr16,
    TinyAsciiStr16Result,
    tsIsLower,
    tsIsUpper,
} from "tinystr";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TinyAsciiStr4` | `class` | 字段: `bytes: [4]u8` · 方法: `empty, fromBytes, fromStr, fromPackedU32, toPackedU32, len, +14 项` |
| `TinyAsciiStr4Result` | `class` | 字段: `ok: bool, value: TinyAsciiStr4, err: i64` |
| `TinyAsciiStr8` | `class` | 字段: `bytes: [8]u8` · 方法: `empty, fromBytes, fromStr, fromPackedU64, toPackedU64, len, +14 项` |
| `TinyAsciiStr8Result` | `class` | 字段: `ok: bool, value: TinyAsciiStr8, err: i64` |
| `TinyAsciiStr16` | `class` | 字段: `bytes: [16]u8` · 方法: `empty, fromBytes, fromStr, len, isEmpty, asBytes, +12 项` |
| `TinyAsciiStr16Result` | `class` | 字段: `ok: bool, value: TinyAsciiStr16, err: i64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `tsIsLower` | `(b: u8): bool` | 调用 `tsIsLower` 并返回 `bool`。 |
| `tsIsUpper` | `(b: u8): bool` | 调用 `tsIsUpper` 并返回 `bool`。 |
| `tsValidateBytes` | `(src: &[]u8, max_len: usize): i64` | 调用 `tsValidateBytes` 并返回 `i64`。 |
| `tsValidatePacked` | `(buf: &[]u8): i64` | 调用 `tsValidatePacked` 并返回 `i64`。 |
| `tsSliceLen` | `(buf: &[]u8): usize` | 调用 `tsSliceLen` 并返回 `usize`。 |
| `tsCopyOut` | `(src: &[]u8, n: usize, out: []u8): usize` | 调用 `tsCopyOut` 并返回 `usize`。 |
| `tsSliceCmp` | `(a: &[]u8, b: &[]u8): i32` | 调用 `tsSliceCmp` 并返回 `i32`。 |
| `tsAllAlpha` | `(buf: &[]u8, n: usize): bool` | 调用 `tsAllAlpha` 并返回 `bool`。 |
| `tsAllAlnum` | `(buf: &[]u8, n: usize): bool` | 调用 `tsAllAlnum` 并返回 `bool`。 |
| `tsAllDigit` | `(buf: &[]u8, n: usize): bool` | 调用 `tsAllDigit` 并返回 `bool`。 |
| `tsAllLower` | `(buf: &[]u8, n: usize): bool` | 调用 `tsAllLower` 并返回 `bool`。 |
| `tsAllUpper` | `(buf: &[]u8, n: usize): bool` | 调用 `tsAllUpper` 并返回 `bool`。 |

## 最小调用示例

```cp
import { tsIsLower } from "tinystr";

pub fn main() {
    let result = tsIsLower(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
