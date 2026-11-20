# `cap:parquet` — Apache Parquet 列式存储与 Thrift / RLE 编解码库

## 1. 概述

`cap:parquet`（亦可经由 `import cap from "cap"; cap.parquet` 访问）是纯 CP 实现的 Apache Parquet 列式存储格式读写库，内置完整的 Thrift `TCompactProtocol` 编解码器与 Parquet `RLE / Bit-Packing Hybrid` 编解码器。所有解析路径均包含魔数校验、Footer 长度边界检查、递归深度上限（`max_depth = 32`）以及防资源耗尽的切片长度校验，确保在畸形输入下安全返回错误且无内存泄漏。

## 2. 核心类型

- `ParquetError`: 错误集合，包含 `InvalidMagic`、`InvalidFooterLength`、`TruncatedData`、`DepthLimitExceeded`、`InvalidThriftType`、`InvalidVarint`、`InvalidRleStream`、`InvalidBitWidth`、`UnsupportedType`、`UnsupportedCompression`、`UnsupportedEncoding`、`InvalidColumnData`、`OutOfMemory`。
- `ParquetType`: 列物理类型枚举（`BOOLEAN`, `INT32`, `INT64`, `INT96`, `FLOAT`, `DOUBLE`, `BYTE_ARRAY`, `FIXED_LEN_BYTE_ARRAY`）。
- `ParquetCompression`: 压缩算法枚举（`UNCOMPRESSED`, `SNAPPY`, `GZIP`, `LZO`, `BROTLI`, `LZ4`, `ZSTD`）。
- `ParquetEncoding`: 页编码枚举（`PLAIN`, `PLAIN_DICTIONARY`, `RLE`, `BIT_PACKED`, `DELTA_BINARY_PACKED`, `RLE_DICTIONARY`）。
- `ColumnSchema`: 列元数据描述（`name`, `col_type`, `repetition_type`）。
- `ColumnData`: 列数据容器，提供 `fromInt32`, `fromInt64`, `fromDouble`, `fromBool`, `fromByteArray` 构造器及 `len()` 方法。
- `ParquetFile`: 解析后的 Parquet 文件对象，包含 `num_rows`, `num_row_groups`, `schemas`, `columns`, `created_by`，需调用 `deinit(alloc)` 释放内存。
- `ThriftWriter` / `ThriftReader` / `ThriftType`: Thrift `TCompactProtocol` 读写器（支持 ZigZag Varint、Delta Field ID、嵌套 Struct/List 及 `MAX_THRIFT_DEPTH = 32` 防护）。

## 3. API 速查

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `writeParquet` | `fn writeParquet(alloc: Allocator, columns: []const ColumnData): ![]u8` | 将多列数据编码为包含 `PAR1` 首尾魔数、DataPage v1 及 `FileMetaData` Footer 的完整 Parquet 字节流 |
| `readParquet` | `fn readParquet(alloc: Allocator, data: []const u8): !ParquetFile` | 校验魔数与 Footer 边界，解析 `FileMetaData` 与各列 DataPage v1 数据 |
| `encodeRleBitPacked` | `fn encodeRleBitPacked(alloc: Allocator, values: []const u32, bit_width: u8): ![]u8` | 执行 Parquet RLE / Bit-Packing Hybrid 混合编码 |
| `decodeRleBitPacked` | `fn decodeRleBitPacked(alloc: Allocator, data: []const u8, bit_width: u8, num_values: usize): ![]u32` | 解码 Parquet RLE / Bit-Packing Hybrid 混合流 |
| `bitWidthForMaxVal` | `fn bitWidthForMaxVal(max_val: u32): u8` | 计算表示 `max_val` 所需的最小比特宽度 |

## 4. 示例

```cp
const std = @import("std");
import { ColumnData, writeParquet, readParquet } from "cap:parquet";

pub fn example(alloc: std.mem.Allocator): !void {
    const ids = [_]i32{ 1, 2, 3 };
    const names = [_][]const u8{ "alice", "bob", "carol" };
    const cols = [_]ColumnData{
        ColumnData.fromInt32("id", &ids),
        ColumnData.fromByteArray("name", &names),
    };

    const bytes = try writeParquet(alloc, &cols);
    defer alloc.free(bytes);

    let file = try readParquet(alloc, bytes);
    defer file.deinit(alloc);
}
```
