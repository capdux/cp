[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# rend

Zero-allocation endian-aware primitive types (U16/U32/I32/U64/I64 Be/Le) and bounds-checked slice readers for zero-copy binary protocols in CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rend` ([https://github.com/rkyv/rend](https://github.com/rkyv/rend), `MIT`)

| 元信息 | 值 |
| --- | --- |
| 包名 | `rend` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add rend
```

```cp
import {
    U16Be,
    U16Le,
    U32Be,
    U32Le,
    I32Be,
    I32Le,
    U64Be,
    U64Le,
} from "rend";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `U16Be` | `class` | 字段: `bytes: [2]u8` · 方法: `fromNative, fromBytes, toNative, asBytes` |
| `U16Le` | `class` | 字段: `bytes: [2]u8` · 方法: `fromNative, fromBytes, toNative, asBytes` |
| `U32Be` | `class` | 字段: `bytes: [4]u8` · 方法: `fromNative, fromBytes, toNative, asBytes` |
| `U32Le` | `class` | 字段: `bytes: [4]u8` · 方法: `fromNative, fromBytes, toNative, asBytes` |
| `I32Be` | `class` | 字段: `bytes: [4]u8` · 方法: `fromNative, fromBytes, toNative, asBytes` |
| `I32Le` | `class` | 字段: `bytes: [4]u8` · 方法: `fromNative, fromBytes, toNative, asBytes` |
| `U64Be` | `class` | 字段: `bytes: [8]u8` · 方法: `fromNative, fromBytes, toNative, asBytes` |
| `U64Le` | `class` | 字段: `bytes: [8]u8` · 方法: `fromNative, fromBytes, toNative, asBytes` |
| `I64Be` | `class` | 字段: `bytes: [8]u8` · 方法: `fromNative, fromBytes, toNative, asBytes` |
| `I64Le` | `class` | 字段: `bytes: [8]u8` · 方法: `fromNative, fromBytes, toNative, asBytes` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `rndPackU16Be` | `(v: u16): [2]u8` | 调用 `rndPackU16Be` 并返回 `[2]u8`。 |
| `rndPackU16Le` | `(v: u16): [2]u8` | 调用 `rndPackU16Le` 并返回 `[2]u8`。 |
| `rndUnpackU16Be` | `(b: [2]u8): u16` | 调用 `rndUnpackU16Be` 并返回 `u16`。 |
| `rndUnpackU16Le` | `(b: [2]u8): u16` | 调用 `rndUnpackU16Le` 并返回 `u16`。 |
| `rndPackU32Be` | `(v: u32): [4]u8` | 调用 `rndPackU32Be` 并返回 `[4]u8`。 |
| `rndPackU32Le` | `(v: u32): [4]u8` | 调用 `rndPackU32Le` 并返回 `[4]u8`。 |
| `rndUnpackU32Be` | `(b: [4]u8): u32` | 调用 `rndUnpackU32Be` 并返回 `u32`。 |
| `rndUnpackU32Le` | `(b: [4]u8): u32` | 调用 `rndUnpackU32Le` 并返回 `u32`。 |
| `rndPackU64Be` | `(v: u64): [8]u8` | 调用 `rndPackU64Be` 并返回 `[8]u8`。 |
| `rndPackU64Le` | `(v: u64): [8]u8` | 调用 `rndPackU64Le` 并返回 `[8]u8`。 |
| `rndUnpackU64Be` | `(b: [8]u8): u64` | 调用 `rndUnpackU64Be` 并返回 `u64`。 |
| `rndUnpackU64Le` | `(b: [8]u8): u64` | 调用 `rndUnpackU64Le` 并返回 `u64`。 |
| `readU16Be` | `(buf: &[]u8, offset: usize): u16?` | 调用 `readU16Be` 并返回 `u16?`。 |
| `readU16Le` | `(buf: &[]u8, offset: usize): u16?` | 调用 `readU16Le` 并返回 `u16?`。 |
| `readU32Be` | `(buf: &[]u8, offset: usize): u32?` | 调用 `readU32Be` 并返回 `u32?`。 |
| `readU32Le` | `(buf: &[]u8, offset: usize): u32?` | 调用 `readU32Le` 并返回 `u32?`。 |
| `readU64Be` | `(buf: &[]u8, offset: usize): u64?` | 调用 `readU64Be` 并返回 `u64?`。 |
| `readU64Le` | `(buf: &[]u8, offset: usize): u64?` | 调用 `readU64Le` 并返回 `u64?`。 |

## 最小调用示例

```cp
import { rndPackU16Be } from "rend";

pub fn main() {
    let result = rndPackU16Be(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
