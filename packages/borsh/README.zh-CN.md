[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# borsh

Zero-allocation canonical little-endian Borsh binary serializer and deserializer for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:borsh` ([https://github.com/near/borsh-rs](https://github.com/near/borsh-rs))

| 元信息 | 值 |
| --- | --- |
| 包名 | `borsh` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add borsh
```

```cp
import {
    BytesSpan,
    BorshWriter,
    BorshReader,
    i32ToBits,
    bitsToI32,
    i64ToBits,
    bitsToI64,
} from "borsh";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BytesSpan` | `class` | 字段: `start: usize, len: usize, ok: bool` |
| `BorshWriter` | `class` | 字段: `pos: usize` · 方法: `new, writeU8, writeBool, writeOptionTag, writeU16Le, writeU32Le, +5 项` |
| `BorshReader` | `class` | 字段: `pos: usize, ok: bool` · 方法: `new, remaining, readU8, readBool, readOptionTag, readU16Le, +5 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `i32ToBits` | `(v: i32): u32` | 调用 `i32ToBits` 并返回 `u32`。 |
| `bitsToI32` | `(u: u32): i32` | 调用 `bitsToI32` 并返回 `i32`。 |
| `i64ToBits` | `(v: i64): u64` | 调用 `i64ToBits` 并返回 `u64`。 |
| `bitsToI64` | `(u: u64): i64` | 调用 `bitsToI64` 并返回 `i64`。 |

## 最小调用示例

```cp
import { i32ToBits } from "borsh";

pub fn main() {
    let result = i32ToBits(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
