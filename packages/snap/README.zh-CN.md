[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# snap

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Google Snappy Raw Block lossless compressor & decompressor with varint32 header, Literal/Copy-1/Copy-2/Copy-4 tags, and overlapping RLE support (Rust snap parity) for CP

## 安装与导入

```bash
cap add snap
```

```cp
import { maxCompressedLen } from "snap";
```

## 核心导出 API

- `export fn maxCompressedLen(input_len: usize): usize;`
- `export fn decodeVarint32(src: &[]u8): VarintRes;`
- `export fn encodeVarint32(val: u32, dst: []u8, start_pos: usize): i64;`
- `export fn uncompressedLen(src: &[]u8): i64;`
- `export fn readU32LE(buf: &[]u8, pos: usize): u32;`
- `export fn hash4(buf: &[]u8, pos: usize): usize;`
- `export fn match4(buf: &[]u8, a: usize, b: usize): bool;`
- `export fn emitLiteral(src: &[]u8, start: usize, len: usize, dst: []u8, start_pos: usize): i64;`
- `export fn emitCopyChunk(offset: usize, chunk_len: usize, dst: []u8, start_pos: usize): i64;`
- `export fn emitCopy(offset: usize, total_len: usize, dst: []u8, start_pos: usize): i64;`
- `export fn compress(src: &[]u8, dst: []u8): i64;`
- `export fn decompress(src: &[]u8, dst: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`snap@1.0.0`)，许可证：BSD-3-Clause
