[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# blake2

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 7693 BLAKE2s-256 streaming cryptographic hash & keyed MAC (Rust blake2 crate parity) for CP

## 安装与导入

```bash
cap add blake2
```

```cp
import { bl2sAdd2 } from "blake2";
```

## 核心导出 API

- `export fn bl2sAdd2(a: u32, b: u32): u32;`
- `export fn bl2sAdd3(a: u32, b: u32, c: u32): u32;`
- `export fn bl2sRotr16(x: u32): u32;`
- `export fn bl2sRotr12(x: u32): u32;`
- `export fn bl2sRotr8(x: u32): u32;`
- `export fn bl2sRotr7(x: u32): u32;`
- `export fn bl2sReadLe32(b: []u8, off: usize): u32;`
- `export fn bl2sWriteLe32(out: []u8, off: usize, v: u32);`
- `export fn bl2sHexDigit(nibble: u8): u8;`
- `export fn bl2sIv(idx: usize): u32;`
- `export fn bl2sMix(v: []u32, ai: usize, bi: usize, ci: usize, di: usize, x: u32, y: u32);`
- `export fn bl2sCompress(h: []u32, block: []u8, t: u64, is_last: bool);`
- `export fn blake2s256Bytes(data: []u8, out32: []u8);`
- `export fn blake2s256Hex(s: string, out64: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`blake2@1.0.0`)，许可证：MIT OR Apache-2.0
