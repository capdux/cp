[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# data-encoding

Zero-allocation strict RFC 4648 Base16/Hex, Base32, and Base64/Base64URL codec with canonical trailing-bit verification (Rust data-encoding v2 parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:data-encoding` ([https://github.com/ia0/data-encoding](https://github.com/ia0/data-encoding))

| 元信息 | 值 |
| --- | --- |
| 包名 | `data-encoding` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add data-encoding
```

```cp
import {
    DecodeResult,
    dencHexDigit,
    dencHexVal,
    dencB32Char,
    dencB32Val,
    dencB64Char,
    dencB64Val,
    dencHexEncodeImpl,
} from "data-encoding";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DecodeResult` | `class` | 字段: `ok: bool, len: usize, error_pos: usize` · 方法: `success, fail` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `dencHexDigit` | `(v: u8, upper: bool): u8` | 调用 `dencHexDigit` 并返回 `u8`。 |
| `dencHexVal` | `(c: u8): u8` | 调用 `dencHexVal` 并返回 `u8`。 |
| `dencB32Char` | `(v: u8): u8` | 调用 `dencB32Char` 并返回 `u8`。 |
| `dencB32Val` | `(c: u8): u8` | 调用 `dencB32Val` 并返回 `u8`。 |
| `dencB64Char` | `(v: u8, url_safe: bool): u8` | 调用 `dencB64Char` 并返回 `u8`。 |
| `dencB64Val` | `(c: u8, url_safe: bool): u8` | 调用 `dencB64Val` 并返回 `u8`。 |
| `dencHexEncodeImpl` | `(src: &[]u8, upper: bool, out: []u8): usize` | 调用 `dencHexEncodeImpl` 并返回 `usize`。 |
| `base64EncodeLen` | `(n: usize, pad: bool): usize` | 调用 `base64EncodeLen` 并返回 `usize`。 |
| `base32EncodeLen` | `(n: usize, pad: bool): usize` | 调用 `base32EncodeLen` 并返回 `usize`。 |
| `hexLowerEncode` | `(src: &[]u8, out: []u8): usize` | 调用 `hexLowerEncode` 并返回 `usize`。 |
| `hexUpperEncode` | `(src: &[]u8, out: []u8): usize` | 调用 `hexUpperEncode` 并返回 `usize`。 |
| `hexDecode` | `(src: &[]u8, out: []u8): DecodeResult` | 调用 `hexDecode` 并返回 `DecodeResult`。 |
| `base32Encode` | `(src: &[]u8, pad: bool, out: []u8): usize` | 调用 `base32Encode` 并返回 `usize`。 |
| `base32Decode` | `(src: &[]u8, out: []u8): DecodeResult` | 调用 `base32Decode` 并返回 `DecodeResult`。 |
| `base64Encode` | `(src: &[]u8, url_safe: bool, pad: bool, out: []u8): usize` | 调用 `base64Encode` 并返回 `usize`。 |
| `base64Decode` | `(src: &[]u8, url_safe: bool, out: []u8): DecodeResult` | 调用 `base64Decode` 并返回 `DecodeResult`。 |

## 最小调用示例

```cp
import { dencHexDigit } from "data-encoding";

pub fn main() {
    let result = dencHexDigit(0, false);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
