[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# lz4_flex

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation LZ4 Raw Block lossless compressor & decompressor with fixed 4096-entry hash table, overlapping RLE match copy, and size-prefixed framing (Rust lz4_flex / LZ4 Block Format parity) for CP

## 安装与导入

```bash
cap add lz4_flex
```

```cp
import { maxCompressedLen } from "lz4_flex";
```

## 核心导出 API

- `export fn maxCompressedLen(input_len: usize): usize;`
- `export fn readU32LE(buf: &[]u8, pos: usize): u32;`
- `export fn hash4(buf: &[]u8, pos: usize): usize;`
- `export fn match4(buf: &[]u8, a: usize, b: usize): bool;`
- `export fn writeExtLen(dst: []u8, pos: usize, extra: usize): i64;`
- `export fn compressBlock(src: &[]u8, dst: []u8): i64;`
- `export fn decompressBlock(src: &[]u8, dst: []u8): i64;`
- `export fn compressWithSize(src: &[]u8, dst: []u8): i64;`
- `export fn decompressWithSize(src: &[]u8, dst: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`lz4_flex@1.0.0`)，许可证：MIT
