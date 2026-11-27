[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ciborium-ll

Zero-heap RFC 8949 low-level streaming CBOR encoder and decoder primitives in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ciborium-ll` ([https://github.com/enarx/ciborium](https://github.com/enarx/ciborium)) — `Apache-2.0`

| 元信息 | 值 |
| --- | --- |
| 包名 | `ciborium-ll` |
| 版本 | `1.0.0` |
| 许可证 | `Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ciborium-ll
```

```cp
import {
    CborHeaderKind,
    CborHeader,
    CborDecoder,
    CborEncoder,
    cblMaxI64AsU64,
    cblReadBe16,
    cblReadBe32,
    cblReadBe64,
} from "ciborium-ll";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `CborHeaderKind` | `Positive, Negative, Bytes, Text, Array, Map, Tag, Bool ...` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CborHeader` | `class` | 字段: `kind: CborHeaderKind, u64_val: u64, i64_val: i64, f64_val: f64, bool_val: bool, len: i64` · 方法: `invalid` |
| `CborDecoder` | `class` | 字段: `data: &[]u8, pos: usize` · 方法: `new, remaining, isEof, readBytes, pull` |
| `CborEncoder` | `class` | 字段: `buf: []u8, pos: usize` · 方法: `new, written, remaining, slice, pushPositive, pushNegativeRaw, +13 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cblMaxI64AsU64` | `(): u64` | 调用 `cblMaxI64AsU64` 并返回 `u64`。 |
| `cblReadBe16` | `(data: &[]u8, pos: usize): u16` | 调用 `cblReadBe16` 并返回 `u16`。 |
| `cblReadBe32` | `(data: &[]u8, pos: usize): u32` | 调用 `cblReadBe32` 并返回 `u32`。 |
| `cblReadBe64` | `(data: &[]u8, pos: usize): u64` | 调用 `cblReadBe64` 并返回 `u64`。 |
| `cblDecodeHalf` | `(bits: u16): f64` | 调用 `cblDecodeHalf` 并返回 `f64`。 |
| `cblArgSize` | `(val: u64): usize` | 调用 `cblArgSize` 并返回 `usize`。 |
| `cblWriteHead` | `(buf: []u8, pos: usize, major: u8, val: u64): usize` | 调用 `cblWriteHead` 并返回 `usize`。 |

## 最小调用示例

```cp
import { cblMaxI64AsU64 } from "ciborium-ll";

pub fn main() {
    let result = cblMaxI64AsU64();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
