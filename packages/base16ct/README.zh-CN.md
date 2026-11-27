[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# base16ct

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP zero-allocation constant-time (table-free) Base16 / Hex encoder and strict decoder (RustCrypto base16ct parity)

## 安装与导入

```bash
cap add base16ct
```

```cp
import { encodedLen } from "base16ct";
```

## 核心导出 API

- `export fn encodedLen(n: usize): usize;`
- `export fn encodeLower(src: []u8, out: []u8): usize;`
- `export fn encodeUpper(src: []u8, out: []u8): usize;`
- `export fn decodeLower(hex: string, out: []u8): HexDecodeResult;`
- `export fn decodeUpper(hex: string, out: []u8): HexDecodeResult;`
- `export fn decodeMixed(hex: string, out: []u8): HexDecodeResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`base16ct@1.0.0`)，许可证：MIT
