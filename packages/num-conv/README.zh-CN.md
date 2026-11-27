[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# num-conv

Zero-heap lossless widening, explicit truncating, checked, and saturating integer conversions matching Rust num-conv in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:num-conv` ([https://github.com/jhpratt/num-conv](https://github.com/jhpratt/num-conv))

| 元信息 | 值 |
| --- | --- |
| 包名 | `num-conv` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add num-conv
```

```cp
import {
    CheckedU8,
    CheckedU16,
    CheckedU32,
    CheckedI8,
    CheckedI16,
    CheckedI32,
    extendU8ToU64,
    extendU16ToU64,
} from "num-conv";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CheckedU8` | `class` | 字段: `val: u8, ok: bool` |
| `CheckedU16` | `class` | 字段: `val: u16, ok: bool` |
| `CheckedU32` | `class` | 字段: `val: u32, ok: bool` |
| `CheckedI8` | `class` | 字段: `val: i8, ok: bool` |
| `CheckedI16` | `class` | 字段: `val: i16, ok: bool` |
| `CheckedI32` | `class` | 字段: `val: i32, ok: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `extendU8ToU64` | `(v: u8): u64` | 调用 `extendU8ToU64` 并返回 `u64`。 |
| `extendU16ToU64` | `(v: u16): u64` | 调用 `extendU16ToU64` 并返回 `u64`。 |
| `extendU32ToU64` | `(v: u32): u64` | 调用 `extendU32ToU64` 并返回 `u64`。 |
| `extendI8ToI64` | `(v: i8): i64` | 调用 `extendI8ToI64` 并返回 `i64`。 |
| `extendI16ToI64` | `(v: i16): i64` | 调用 `extendI16ToI64` 并返回 `i64`。 |
| `extendI32ToI64` | `(v: i32): i64` | 调用 `extendI32ToI64` 并返回 `i64`。 |
| `truncateU64ToU8` | `(v: u64): u8` | 调用 `truncateU64ToU8` 并返回 `u8`。 |
| `truncateU64ToU16` | `(v: u64): u16` | 调用 `truncateU64ToU16` 并返回 `u16`。 |
| `truncateU64ToU32` | `(v: u64): u32` | 调用 `truncateU64ToU32` 并返回 `u32`。 |
| `tryU64ToU8` | `(v: u64): CheckedU8` | 调用 `tryU64ToU8` 并返回 `CheckedU8`。 |
| `tryU64ToU16` | `(v: u64): CheckedU16` | 调用 `tryU64ToU16` 并返回 `CheckedU16`。 |
| `tryU64ToU32` | `(v: u64): CheckedU32` | 调用 `tryU64ToU32` 并返回 `CheckedU32`。 |
| `tryI64ToI8` | `(v: i64): CheckedI8` | 调用 `tryI64ToI8` 并返回 `CheckedI8`。 |
| `tryI64ToI16` | `(v: i64): CheckedI16` | 调用 `tryI64ToI16` 并返回 `CheckedI16`。 |
| `tryI64ToI32` | `(v: i64): CheckedI32` | 调用 `tryI64ToI32` 并返回 `CheckedI32`。 |
| `tryI64ToU8` | `(v: i64): CheckedU8` | 调用 `tryI64ToU8` 并返回 `CheckedU8`。 |
| `tryI64ToU32` | `(v: i64): CheckedU32` | 调用 `tryI64ToU32` 并返回 `CheckedU32`。 |
| `saturatingI64ToU8` | `(v: i64): u8` | 调用 `saturatingI64ToU8` 并返回 `u8`。 |
| `saturatingI64ToI8` | `(v: i64): i8` | 调用 `saturatingI64ToI8` 并返回 `i8`。 |
| `saturatingI64ToI16` | `(v: i64): i16` | 调用 `saturatingI64ToI16` 并返回 `i16`。 |
| `saturatingU64ToU32` | `(v: u64): u32` | 调用 `saturatingU64ToU32` 并返回 `u32`。 |

## 最小调用示例

```cp
import { extendU8ToU64 } from "num-conv";

pub fn main() {
    let result = extendU8ToU64(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
