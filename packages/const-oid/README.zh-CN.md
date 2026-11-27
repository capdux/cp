[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# const-oid

Pure CP zero-allocation ITU-T X.690 ASN.1 Object Identifier (OID) parser, encoder, and comparator (parity with RustCrypto const-oid)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`const-oid`](https://github.com/RustCrypto/formats)（`MIT OR Apache-2.0`）（包名 `const-oid`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `const-oid` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add const-oid
```

```cp
import {
    OidParseResult,
    ObjectIdentifier,
    coidZeroArcs,
    coidZeroDer,
    coidEncodeBase128U64,
    coidEncodeArcsToDer,
    coidWriteU32Dec,
} from "const-oid";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `OidParseResult` | `class` | 字段: `ok: bool, oid: ObjectIdentifier` |
| `ObjectIdentifier` | `class` | 字段: `arcs: [16]u32, len: usize, der_buf: [80]u8, der_len: usize` · 方法: `empty, fromStr, fromDer, fromDerTlv, toDer, toDerTlv, +6 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `coidZeroArcs` | `(): [16]u32` | 调用 `coidZeroArcs` 并返回 `[16]u32`。 |
| `coidZeroDer` | `(): [80]u8` | 调用 `coidZeroDer` 并返回 `[80]u8`。 |
| `coidEncodeBase128U64` | `(val: u64, out: []u8, start_pos: usize): usize` | 调用 `coidEncodeBase128U64` 并返回 `usize`。 |
| `coidEncodeArcsToDer` | `(arcs: [16]u32, arc_len: usize, der_out: []u8): usize` | 调用 `coidEncodeArcsToDer` 并返回 `usize`。 |
| `coidWriteU32Dec` | `(val: u32, out: []u8, start_pos: usize): usize` | 调用 `coidWriteU32Dec` 并返回 `usize`。 |

## 最小调用示例

```cp
import { coidZeroArcs } from "const-oid";

pub fn main() {
    let result = coidZeroArcs();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
