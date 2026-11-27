[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# sha1

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation FIPS 180-4 SHA-1 cryptographic hash streaming implementation (Rust sha1 crate parity) for CP

## 安装与导入

```bash
cap add sha1
```

```cp
import { sha1Add32 } from "sha1";
```

## 核心导出 API

- `export fn sha1Add32(a: u32, b: u32): u32;`
- `export fn sha1Add5(a: u32, b: u32, c: u32, d: u32, e: u32): u32;`
- `export fn sha1Rotl1(x: u32): u32;`
- `export fn sha1Rotl5(x: u32): u32;`
- `export fn sha1Rotl30(x: u32): u32;`
- `export fn sha1ReadBe32(b: []u8, off: usize): u32;`
- `export fn sha1WriteBe32(out: []u8, off: usize, v: u32);`
- `export fn sha1HexDigit(nibble: u8): u8;`
- `export fn sha1Bytes(data: []u8, out20: []u8);`
- `export fn sha1Hex(s: string, out40: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`sha1@1.0.0`)，许可证：MIT OR Apache-2.0
