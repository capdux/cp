[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# data-encoding

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation strict RFC 4648 Base16/Hex, Base32, and Base64/Base64URL codec with canonical trailing-bit verification (Rust data-encoding v2 parity) for CP

## 安装与导入

```bash
cap add data-encoding
```

```cp
import { dencHexDigit } from "data-encoding";
```

## 核心导出 API

- `export fn dencHexDigit(v: u8, upper: bool): u8;`
- `export fn dencHexVal(c: u8): u8;`
- `export fn dencB32Char(v: u8): u8;`
- `export fn dencB32Val(c: u8): u8;`
- `export fn dencB64Char(v: u8, url_safe: bool): u8;`
- `export fn dencB64Val(c: u8, url_safe: bool): u8;`
- `export fn dencHexEncodeImpl(src: &[]u8, upper: bool, out: []u8): usize;`
- `export fn base64EncodeLen(n: usize, pad: bool): usize;`
- `export fn base32EncodeLen(n: usize, pad: bool): usize;`
- `export fn hexLowerEncode(src: &[]u8, out: []u8): usize;`
- `export fn hexUpperEncode(src: &[]u8, out: []u8): usize;`
- `export fn hexDecode(src: &[]u8, out: []u8): DecodeResult;`
- `export fn base32Encode(src: &[]u8, pad: bool, out: []u8): usize;`
- `export fn base32Decode(src: &[]u8, out: []u8): DecodeResult;`
- `export fn base64Encode(src: &[]u8, url_safe: bool, pad: bool, out: []u8): usize;`
- `export fn base64Decode(src: &[]u8, url_safe: bool, out: []u8): DecodeResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`data-encoding@1.0.0`)，许可证：MIT
