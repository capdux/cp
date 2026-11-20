# `cap:kafka_wire` — Apache Kafka Wire Protocol & RecordBatch v2 标准库

## 1. 概述

`cap:kafka_wire`（支持 `import { ... } from "cap:kafka_wire"` 及 `import cap from "cap"` 动态挂载 `cap.kafka_wire`）是纯 CP 实现的工业级 Apache Kafka 线协议编解码与 RecordBatch v2（CRC32C Castagnoli）标准库。

核心能力：
- **二进制原语**：大端定长整数（`i8`/`i16`/`i32`/`i64`/`u8`/`u16`/`u32`/`u64`）、Unsigned Varint (`u32`/`u64`)、ZigZag Signed Varint (`i32`/`i64`)、Nullable String、Compact String / Compact Bytes / Compact Array（`N+1` 编码与 `0` 表示 `null`）、Tagged Fields（`_tagged_fields`）跳过与空标记编码。
- **协议帧与核心 API**：`RequestHeader` / `ResponseHeader`（标准与 Flexible 版本）、`ApiVersions` (Key 18, v0..v3)、`Metadata` (Key 3, Broker/Topic/Partition 拓扑) 以及带 `max_frame_bytes` 防资源耗尽保护的 4 字节大端长度前缀 `KafkaStreamFramer`。
- **RecordBatch v2 (`magic = 2`)**：完整 61 字节批头与变长 Record/Header 编解码，使用 `std.hash.crc.Crc32Iscsi` 自动计算并校验从 `attributes` 字节起至批尾的 CRC32C。

---

## 2. 核心类型

| 类型 | 说明 |
| :--- | :--- |
| `KafkaError` | 错误集：`UnexpectedEof`, `InvalidVarint`, `InvalidLength`, `InvalidMagic`, `CrcMismatch`, `FrameTooLarge`, `UnsupportedVersion`, `OutOfMemory` |
| `KafkaReader` | 零拷贝切片顺序解码器，维护只读缓冲区与游标 `pos` |
| `KafkaWriter` | 基于 `std.ArrayList(u8)` 的动态编码器，支持导出拥有所有权的字节切片 |
| `ApiKey` | Kafka API Key 枚举（`Produce=0`, `Fetch=1`, `Metadata=3`, `ApiVersions=18` 等） |
| `RequestHeader` / `ResponseHeader` | 请求/响应公共帧头（支持 Flexible Tagged Fields） |
| `ApiVersionsReq` / `ApiVersionsResp` / `ApiVersionRange` | `ApiVersions` (Key 18) 协议请求与响应 |
| `MetadataReq` / `MetadataResp` / `BrokerMeta` / `TopicMeta` / `PartitionMeta` | `Metadata` (Key 3) 集群拓扑请求与响应 |
| `KafkaStreamFramer` | 4 字节大端长度前缀流式拆帧器，支持任意半包/黏包切片与 `max_frame_bytes` 拦截 |
| `RecordBatch` / `Record` / `RecordHeader` | Kafka RecordBatch v2 (`magic = 2`) 批结构与单条记录结构 |

---

## 3. API 速查

- **`KafkaReader.init(buf)`**：构造解码器；常用方法 `readI8/16/32/64`, `readUVarint`, `readVarint`, `readVarlong`, `readNullableString`, `readCompactString`, `readCompactArrayLen`, `readVarintBytes`, `skipTaggedFields`。
- **`KafkaWriter.init(alloc)`**：构造编码器；`deinit()` 释放，`toOwnedSlice()` 移交所有权；常用方法 `writeI8/16/32/64`, `writeUVarint`, `writeVarint`, `writeVarlong`, `writeNullableString`, `writeCompactString`, `writeCompactArrayLen`, `writeVarintBytes`, `writeEmptyTaggedFields`。
- **`KafkaStreamFramer.init(alloc, max_frame_bytes)`**：`push(chunk)` 追加任意长度网络分片，`nextFrame()` 返回完整帧（需调用方 `alloc.free`）或 `null`；`framePayload(alloc, payload)` 封装 4 字节长度前缀。
- **`encodeRecordBatch(alloc, &batch)` / `decodeRecordBatch(alloc, data)`**：编码时自动回填 CRC32C；解码时严格校验 `magic == 2`、CRC32C 与长度边界（`batch.deinit(alloc)` 释放）。

---

## 4. 示例

```cp
const std = @import("std");
import { KafkaWriter, KafkaReader, encodeRecordBatch, decodeRecordBatch, RecordBatch, Record, RecordHeader } from "cap:kafka_wire";

fn roundtrip(alloc: std.mem.Allocator): !void {
    const hdrs = [_]RecordHeader{
        RecordHeader{ .key = "trace-id", .value = "abc-123" },
    };
    const recs = [_]Record{
        Record{
            .attributes = 0,
            .timestamp_delta = 0,
            .offset_delta = 0,
            .key = "k1",
            .value = "hello-kafka",
            .headers = &hdrs,
        },
    };
    const batch = RecordBatch{
        .base_offset = 100,
        .partition_leader_epoch = 1,
        .attributes = 0,
        .last_offset_delta = 0,
        .base_timestamp = 1700000000000,
        .max_timestamp = 1700000000000,
        .producer_id = -1,
        .producer_epoch = -1,
        .base_sequence = -1,
        .records = &recs,
    };
    const encoded = encodeRecordBatch(alloc, &batch)?;
    defer alloc.free(encoded);

    let decoded = decodeRecordBatch(alloc, encoded)?;
    defer decoded.deinit(alloc);
}
```
