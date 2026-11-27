[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/lz4

> 版本：`1.10.0` | 许可证：`BSD-2-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Extremely fast lossless compression (LZ4 & LZ4-HC) native C binding for CP

## 安装与导入

```bash
cap add @cap/lz4
```

```cp
import { LZ4_versionNumber } from "@cap/lz4";
```

## 核心导出 API

- `export fn LZ4_versionNumber(): i32;`
- `export fn LZ4_compressBound(inputSize: i32): i32;`
- `export fn LZ4_compress_default(src: [*]&u8!, dst: [*]u8!, srcSize: i32, dstCapacity: i32): i32;`
- `export fn LZ4_compress_fast(src: [*]&u8!, dst: [*]u8!, srcSize: i32, dstCapacity: i32, acceleration: i32): i32;`
- `export fn LZ4_compress_HC(src: [*]&u8!, dst: [*]u8!, srcSize: i32, dstCapacity: i32, compressionLevel: i32): i32;`
- `export fn LZ4_decompress_safe(src: [*]&u8!, dst: [*]u8!, compressedSize: i32, dstCapacity: i32): i32;`
- `export fn versionNumber(): i32;`
- `export fn compressBound(input_len: usize): !usize;`
- `export fn compressInto(src: &[]u8, dst: []u8): !&[]u8;`
- `export fn compressFastInto(src: &[]u8, dst: []u8, acceleration: i32): !&[]u8;`
- `export fn compressHCInto(src: &[]u8, dst: []u8, level: i32): !&[]u8;`
- `export fn decompressInto(src: &[]u8, dst: []u8): !&[]u8;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`lz4@1.10.0`)，许可证：BSD-2-Clause
