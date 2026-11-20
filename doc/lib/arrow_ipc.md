# `cap:arrow_ipc` — Apache Arrow IPC 列式内存批处理与零拷贝读写标准库

## 1. 概述

`cap:arrow_ipc`（及 `cap.arrow_ipc`）提供纯 CP 实现的工业级 Apache Arrow 列式内存数据模型、空值位图（Validity Bitmap）、轻量确定性 Flatbuffer 元数据编解码器，以及完整的 Arrow IPC Streaming 与 File Format (`"ARROW1"`) 序列化/反序列化能力。所有解码路径均内置严格边界检查，防止畸形魔数、Footer/Buffer 偏移越界、行数不一致与非法 UTF-8 偏移导致内存越界或泄漏。

## 2. 核心类型

- **`ArrowError`**：错误集合，包含 `OutOfMemory`、`InvalidMagic`、`InvalidFooter`、`InvalidFlatbuffer`、`InvalidContinuation`、`UnexpectedEof`、`RowCountMismatch`、`ColumnCountMismatch`、`TypeMismatch`、`IndexOutOfBounds`、`InvalidOffset`、`BufferOutOfBounds`、`InvalidUtf8`、`EmptySchema`。
- **`ArrowType`**：支持 `Int32`、`Int64`、`Float64`、`Bool`、`Utf8`。
- **`ArrowField` / `ArrowSchema`**：字段定义与不可变模式描述，支持 `init`、`clone`、`deinit`。
- **`ValidityBitmap`**：LSB-first 空值位图，支持 `initAllValid`、`initFromBools`、`fromRawBytes`、`isValid`、`setValid`、`deinit`。
- **`ArrowColumn`**：列式存储容器，支持定长数值、bit-packed `Bool` 及 32-bit offsets + 连续数据缓冲区的 `Utf8`。
- **`RecordBatch`**：列式批处理单元，强制校验各列行数一致性、类型匹配及 `null_count` 准确性。
- **`ArrowStreamResult`**：流式/文件读取结果（包含 `schema` 与 `batches`），提供 `deinit(alloc)` 一键零泄漏释放。

## 3. API 速查

| 类别 | 符号 | 说明 |
| --- | --- | --- |
| 列构建 | `ArrowColumn.fromInt32 / fromInt64 / fromFloat64 / fromBool / fromUtf8` | 从切片与可选有效性掩码构建列 |
| 列查询 | `col.getInt32 / getInt64 / getFloat64 / getBool / getUtf8(idx)` | 返回 `(T)?`，空值返回 `null` |
| 批处理 | `RecordBatch.init(alloc, schema, columns)` | 校验并封装 `RecordBatch` |
| Streaming | `writeStream(alloc, &schema, batches)` / `readStream(alloc, bytes)` | Arrow IPC 流式格式读写（含 EOS `0xFFFFFFFF 0x00000000`） |
| File Format | `writeFile(alloc, &schema, batches)` / `readFile(alloc, bytes)` | Arrow `"ARROW1"` 文件格式读写（含首尾魔数与 Footer） |
| Flatbuffer | `encodeSchemaMessage` / `encodeRecordBatchMessage` / `encodeFooter` / `decodeMessage` / `decodeFooter` | 轻量确定性 Flatbuffer 元数据编解码 |

## 4. 示例

```typescript
import {
    ArrowField,
    ArrowSchema,
    ArrowColumn,
    RecordBatch,
    writeFile,
    readFile,
} from "cap:arrow_ipc";

const std = @import("std");

pub fn example(alloc: std.mem.Allocator): !void {
    const fields: [2]ArrowField = [
        ArrowField.init("id", .Int32, false),
        ArrowField.init("name", .Utf8, true),
    ];
    let schema = try ArrowSchema.init(alloc, &fields);
    defer schema.deinit(alloc);

    const ids: [2]i32 = [1, 2];
    const names: [2][]const u8 = ["alice", ""];
    const valid: [2]bool = [true, false];

    let cols: [2]ArrowColumn = [
        try ArrowColumn.fromInt32(alloc, "id", &ids, null),
        try ArrowColumn.fromUtf8(alloc, "name", &names, &valid),
    ];
    let batch = try RecordBatch.init(alloc, schema, &cols);
    defer batch.deinit(alloc);

    const batches: [1]RecordBatch = [batch];
    const file_bytes = try writeFile(alloc, &schema, &batches);
    defer alloc.free(file_bytes);

    let decoded = try readFile(alloc, file_bytes);
    defer decoded.deinit(alloc);
}
```
