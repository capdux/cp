[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# unsigned-varint

Zero-allocation 3-in-1 variable-length integer codec (ULEB128, SLEB128, Protobuf ZigZag, and QUIC RFC 9000 varint) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unsigned-varint` ([https://github.com/paritytech/unsigned-varint](https://github.com/paritytech/unsigned-varint))（原库名 `unsigned-varint` → CP 包名 `unsigned-varint`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `unsigned-varint` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add unsigned-varint
```

```cp
import {
    U64Decode,
    I64Decode,
    i64ToBits,
    bitsToI64,
    uleb128Size,
    encodeUleb128,
    decodeUleb128,
    zigzagEncode,
} from "unsigned-varint";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `U64Decode` | `class` | 字段: `ok: bool, value: u64, bytes_read: usize` · 方法: `fail` |
| `I64Decode` | `class` | 字段: `ok: bool, value: i64, bytes_read: usize` · 方法: `fail` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `i64ToBits` | `(v: i64): u64` | 调用 `i64ToBits` 并返回 `u64`。 |
| `bitsToI64` | `(u: u64): i64` | 调用 `bitsToI64` 并返回 `i64`。 |
| `uleb128Size` | `(v: u64): usize` | 调用 `uleb128Size` 并返回 `usize`。 |
| `encodeUleb128` | `(v: u64, out: []u8): usize?` | 通过 `encodeUleb128` 编码或格式化输出，返回 `usize?`。 |
| `decodeUleb128` | `(data: string): U64Decode` | 通过 `decodeUleb128` 解析输入数据，返回 `U64Decode`。 |
| `zigzagEncode` | `(v: i64): u64` | 调用 `zigzagEncode` 并返回 `u64`。 |
| `zigzagDecode` | `(u: u64): i64` | 调用 `zigzagDecode` 并返回 `i64`。 |
| `encodeSleb128` | `(v: i64, out: []u8): usize?` | 通过 `encodeSleb128` 编码或格式化输出，返回 `usize?`。 |
| `decodeSleb128` | `(data: string): I64Decode` | 通过 `decodeSleb128` 解析输入数据，返回 `I64Decode`。 |
| `encodeZigzagVarint` | `(v: i64, out: []u8): usize?` | 通过 `encodeZigzagVarint` 编码或格式化输出，返回 `usize?`。 |
| `decodeZigzagVarint` | `(data: string): I64Decode` | 通过 `decodeZigzagVarint` 解析输入数据，返回 `I64Decode`。 |
| `encodeQuicVarint` | `(v: u64, out: []u8): usize?` | 通过 `encodeQuicVarint` 编码或格式化输出，返回 `usize?`。 |
| `decodeQuicVarint` | `(data: string): U64Decode` | 通过 `decodeQuicVarint` 解析输入数据，返回 `U64Decode`。 |

## 最小调用示例

```cp
import { decodeUleb128 } from "unsigned-varint";

pub fn main() {
    let result = decodeUleb128("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
