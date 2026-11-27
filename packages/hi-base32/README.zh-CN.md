[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# hi-base32

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 4648 Section 6/7 standard Base32 (A-Z2-7) and Base32hex (0-9A-V) encoder and decoder in pure CP

## 安装与导入

```bash
cap add hi-base32
```

```cp
import { decodeStdChar } from "hi-base32";
```

## 核心导出 API

- `export fn decodeStdChar(b: u8): i32;`
- `export fn decodeHexChar(b: u8): i32;`
- `export fn encodedLen(byte_len: usize, pad: bool): usize;`
- `export fn encodeWithAlphabet(data: &[]u8, pad: bool, alphabet: string, out: []u8): usize;`
- `export fn decodeWithMode(input: string, is_hex: bool, out: []u8): i64;`
- `export fn encode(data: &[]u8, pad: bool, out: []u8): usize;`
- `export fn decode(input: string, out: []u8): i64;`
- `export fn encodeHex(data: &[]u8, pad: bool, out: []u8): usize;`
- `export fn decodeHex(input: string, out: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`hi-base32@1.0.0`)，许可证：MIT
