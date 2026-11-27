[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# der

Zero-allocation ITU-T X.690 ASN.1 DER encoder and strict decoder reusing const-oid for OID TLV handling (RustCrypto der parity) in pure CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`der`](https://github.com/RustCrypto/formats)（`MIT OR Apache-2.0`）（原库名 `der`，CP 包名 `der`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `der` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add der
```

```cp
import {
    DerHeader,
    DerTlvResult,
    DerSliceResult,
    DerBoolResult,
    DerU64Result,
    DerBitStringResult,
    DerSequenceResult,
    DerReader,
} from "der";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DerHeader` | `class` | 字段: `tag: u8, len: usize, header_len: usize, ok: bool` |
| `DerTlvResult` | `class` | 字段: `tag: u8, value: &[]u8, ok: bool` |
| `DerSliceResult` | `class` | 字段: `data: &[]u8, ok: bool` |
| `DerBoolResult` | `class` | 字段: `value: bool, ok: bool` |
| `DerU64Result` | `class` | 字段: `value: u64, ok: bool` |
| `DerBitStringResult` | `class` | 字段: `unused_bits: u8, bytes: &[]u8, ok: bool` |
| `DerSequenceResult` | `class` | 字段: `reader: DerReader, ok: bool` |
| `DerReader` | `class` | 字段: `buf: &[]u8, pos: usize` · 方法: `new, remaining, isEmpty, peekTag, readHeader, readTlv, +7 项` |
| `DerWriter` | `class` | 字段: `buf: []u8, pos: usize` · 方法: `new, len, remaining, written, encodedLengthSize, encodeLength, +8 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `derEmptySlice` | `(): &[]u8` | 调用 `derEmptySlice` 并返回 `&[]u8`。 |
| `derMinBytesUsize` | `(v: usize): usize` | 调用 `derMinBytesUsize` 并返回 `usize`。 |

## 最小调用示例

```cp
import { derEmptySlice } from "der";

pub fn main() {
    let result = derEmptySlice();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
