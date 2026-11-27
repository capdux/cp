[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/snappy

> 版本：`1.2.1` | 许可证：`BSD-3-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Google Snappy 1.2.1 fast compression/decompression library (Kafka/Parquet/LevelDB/RocksDB) for CP

## 安装与导入

```bash
cap add @cap/snappy
```

```cp
import { snappy_compress } from "@cap/snappy";
```

## 核心导出 API

- `export fn snappy_compress(input: [*]&u8!, input_length: usize, compressed: [*]u8!, compressed_length: *usize!): i32;`
- `export fn snappy_uncompress(compressed: [*]&u8!, compressed_length: usize, uncompressed: [*]u8!, uncompressed_length: *usize!): i32;`
- `export fn snappy_max_compressed_length(source_length: usize): usize;`
- `export fn snappy_uncompressed_length(compressed: [*]&u8!, compressed_length: usize, result: *usize!): i32;`
- `export fn snappy_validate_compressed_buffer(compressed: [*]&u8!, compressed_length: usize): i32;`
- `export fn maxCompressedLength(source_len: usize): usize;`
- `export fn uncompressedLength(compressed: &[]u8): !usize;`
- `export fn compressInto(src: &[]u8, dst: []u8): !&[]u8;`
- `export fn uncompressInto(compressed: &[]u8, dst: []u8): !&[]u8;`
- `export fn validateCompressedBuffer(compressed: &[]u8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c++` (`snappy@1.2.1`)，许可证：BSD-3-Clause
