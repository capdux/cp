[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# bech32

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation BIP-173 Bech32 and BIP-350 Bech32m encoder, decoder, 8-to-5 bit converter, and BCH checksum verifier for CP

## 安装与导入

```bash
cap add bech32
```

```cp
import { toLowerAscii } from "bech32";
```

## 核心导出 API

- `export enum Bech32Variant`
- `export fn toLowerAscii(c: u8): u8;`
- `export fn charsetIndex(c: u8): u8?;`
- `export fn polymodStep(chk: u32, v: u8): u32;`
- `export fn polymodHrp(hrp: string): u32;`
- `export fn variantConst(v: Bech32Variant): u32;`
- `export fn isValidHrp(hrp: string): bool;`
- `export fn to5BitWords(bytes: string, out: []u8): usize?;`
- `export fn from5BitWords(words: &[]u8, out: []u8): usize?;`
- `export fn encodeBech32(hrp: string, payload_bytes: string, variant: Bech32Variant, out: []u8): string?;`
- `export fn validateStructureAndChecksum(encoded: string): ParsedBech32Meta;`
- `export fn verifyBech32(encoded: string): bool;`
- `export fn decodeBech32(encoded: string, hrp_out: []u8, data_out: []u8): Bech32Decoded;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`bech32@1.0.0`)，许可证：MIT
