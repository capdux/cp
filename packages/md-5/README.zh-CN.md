[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# md-5

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 1321 MD5 cryptographic hash streaming implementation (Rust md-5/md5 crate parity) for CP

## 安装与导入

```bash
cap add md-5
```

```cp
import { md5Add32 } from "md-5";
```

## 核心导出 API

- `export fn md5Add32(a: u32, b: u32): u32;`
- `export fn md5Add4(a: u32, b: u32, c: u32, d: u32): u32;`
- `export fn md5Rotl32(x: u32, bits: u32): u32;`
- `export fn md5ShiftAt(idx: usize): u32;`
- `export fn md5ConstAt(idx: usize): u32;`
- `export fn md5ReadLe32(b: []u8, off: usize): u32;`
- `export fn md5WriteLe32(out: []u8, off: usize, v: u32);`
- `export fn md5HexDigit(nibble: u8): u8;`
- `export fn md5Bytes(data: []u8, out16: []u8);`
- `export fn md5Hex(s: string, out32: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`md-5@1.0.0`)，许可证：MIT OR Apache-2.0
