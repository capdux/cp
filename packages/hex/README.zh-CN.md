[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# hex

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation hexadecimal encoder/decoder, fixed-width integer hex formatter, and canonical HexDump utility for CP

## 安装与导入

```bash
cap add hex
```

```cp
import { parseNibble } from "hex";
```

## 核心导出 API

- `export fn parseNibble(c: u8): u8?;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn appendStr(out: []u8, pos: usize, s: string): usize;`
- `export fn encodeHex(input: string, out: []u8): string;`
- `export fn encodeHexUpper(input: string, out: []u8): string;`
- `export fn isValidHex(s: string): bool;`
- `export fn decodeHex(hex_str: string, out: []u8): string?;`
- `export fn decodeHexPrefixed(hex_str: string, out: []u8): string?;`
- `export fn encodeU32Hex(v: u32, out: []u8): string;`
- `export fn encodeU64Hex(v: u64, out: []u8): string;`
- `export fn hexDump(data: string, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`hex@1.0.0`)，许可证：MIT
