[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# base16ct

Pure CP zero-allocation constant-time (table-free) Base16 / Hex encoder and strict decoder (RustCrypto base16ct parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:base16ct` ([https://github.com/RustCrypto/formats](https://github.com/RustCrypto/formats))

| 元信息 | 值 |
| --- | --- |
| 包名 | `base16ct` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add base16ct
```

```cp
import {
    HexDecodeResult,
    b16InRange,
    b16EncodeNibbleLower,
    b16EncodeNibbleUpper,
    b16DecodeNibble,
    b16ZeroOut,
    encodedLen,
    encodeLower,
} from "base16ct";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `HexDecodeResult` | `class` | 字段: `len: usize, ok: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `b16InRange` | `(c: u32, lo: u32, hi: u32): u32` | 调用 `b16InRange` 并返回 `u32`。 |
| `b16EncodeNibbleLower` | `(nib: u8): u8` | 调用 `b16EncodeNibbleLower` 并返回 `u8`。 |
| `b16EncodeNibbleUpper` | `(nib: u8): u8` | 调用 `b16EncodeNibbleUpper` 并返回 `u8`。 |
| `b16DecodeNibble` | `(ch: u8, allow_lower: bool, allow_upper: bool): u32` | 调用 `b16DecodeNibble` 并返回 `u32`。 |
| `b16ZeroOut` | `(out: []u8, n: usize)` | 就地执行 `b16ZeroOut` 操作。 |
| `encodedLen` | `(n: usize): usize` | 通过 `encodedLen` 编码或格式化输出，返回 `usize`。 |
| `encodeLower` | `(src: []u8, out: []u8): usize` | 通过 `encodeLower` 编码或格式化输出，返回 `usize`。 |
| `encodeUpper` | `(src: []u8, out: []u8): usize` | 通过 `encodeUpper` 编码或格式化输出，返回 `usize`。 |
| `b16DecodeCore` | `(hex: string, out: []u8, allow_lower: bool, allow_upper: bool): HexDecodeResult` | 调用 `b16DecodeCore` 并返回 `HexDecodeResult`。 |
| `decodeLower` | `(hex: string, out: []u8): HexDecodeResult` | 通过 `decodeLower` 解析输入数据，返回 `HexDecodeResult`。 |
| `decodeUpper` | `(hex: string, out: []u8): HexDecodeResult` | 通过 `decodeUpper` 解析输入数据，返回 `HexDecodeResult`。 |
| `decodeMixed` | `(hex: string, out: []u8): HexDecodeResult` | 通过 `decodeMixed` 解析输入数据，返回 `HexDecodeResult`。 |

## 最小调用示例

```cp
import { decodeLower } from "base16ct";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decodeLower("example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
