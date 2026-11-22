# `cap:binary_schema` — 纯 CP 紧凑二进制 Schema 流式编解码引擎

> 参考来源：/data/dev/bun/src/analytics/schema.cp | 吸收日期：2026-11-22

## 概述

`cap:binary_schema` 吸收自 Bun 遥测与结构化二进制序列化引擎（`src/analytics/schema.cp`），提供跨平台确定性小端二进制流式读取器 `SchemaReader`、可增长 RAII 堆写入器 `SchemaWriter`（`~init()` 自动释放）、零堆分配固定缓冲区写入器 `FixedSchemaWriter` 以及双 32 位紧凑整型 `CompactUint64`，支持整型、布尔、长度前缀字节数组/字符串、泛型枚举合法性校验以及带字段 ID 的向后兼容结构体记录编解码。

## 上游编码契约与安全防御

- **字段 ID + 结束哨兵协议（`END_OF_MESSAGE = 0`）**：每条结构化消息按 `[field_id: u8][payload]...` 顺序写入，并以 `0`（`END_OF_MESSAGE`）终结。该设计允许发送端省略默认值字段实现紧凑编码，同时为协议演进提供清晰的消息边界。
- **确定性小端编码**：所有定宽整型与 `u32` 长度前缀均采用显式小端（Little-Endian）读写，消除大小端架构差异。
- **枚举合法性防 UB 校验（`readEnum`）**：吸收自上游 `Reader.readEnum`，反序列化枚举时先读取底层整数 tag，再通过 `comptime` 遍历枚举声明的所有合法成员进行比对；若收到未知 tag 直接返回 `SchemaError.InvalidEnumValue`，杜绝非法整数强转枚举导致的未定义行为。
- **防越界与零拷贝读取**：`SchemaReader` 每次读取前严格比对 `remaining()`，任何截断或恶意超大 `u32` 长度前缀都会立即返回 `SchemaError.UnexpectedEof`；`readBytes`/`readByteArray`/`readString` 直接返回输入缓冲的零拷贝切片。

## 核心类型与 API

### 常量与错误集

- `END_OF_MESSAGE: u8 = 0`（字段 ID 结束哨兵）
- `SchemaError`：`UnexpectedEof` / `InvalidEnumValue` / `InvalidFieldId` / `BufferTooSmall` / `OutOfMemory`

### `SchemaReader`（零拷贝读取器）

- `SchemaReader.init(data: []const u8): SchemaReader`
- 状态查询：`remaining()`、`offset()`、`isEof()`
- 基本读取：`readBytes(count)`、`readByte()`、`readBool()`、`readI8()`、`readU16()`、`readI16()`、`readU32()`、`readI32()`、`readU64()`、`readI64()`、`readInt(T)`
- 长度前缀与结构化读取：`readByteArray()`、`readString()`、`readFieldId()`、`readEnum(Enum)`

### `SchemaWriter`（RAII 可增长写入器）

- `SchemaWriter.init(allocator)` / `SchemaWriter.initCapacity(allocator, initial_cap)`
- 查询与重置：`bytes()`、`written()`、`reset()`
- 写入方法：`writeBytes`、`writeByte`、`writeBool`、`writeI8`/`writeU16`/`writeI16`/`writeU32`/`writeI32`/`writeU64`/`writeI64`/`writeInt(T, v)`、`writeByteArray`、`writeString`、`writeFieldId`、`endMessage`、`writeEnum(Enum, val)`
- 自动析构：`~init()` 自动释放堆缓冲

### `FixedSchemaWriter` 与 `CompactUint64`

- `FixedSchemaWriter.init(buf: []u8)`：栈上零堆分配写入器，缓冲不足返回 `SchemaError.BufferTooSmall`
- `CompactUint64`：将 `u64` 拆分为高低双 `u32` 的兼容封装，提供 `fromU64(v)`、`toU64()`、`encode(*SchemaWriter)`、`decode(*SchemaReader)`

## 边界注意事项

1. **切片生命周期**：`SchemaReader.readString()` / `readByteArray()` 返回的切片借用自底层 `data`，消费期间不得释放原缓冲区。
2. **字段 ID `0` 保留**：`0` 专用于 `END_OF_MESSAGE`，业务字段 ID 应从 `1..255` 分配。

## 示例

```cp
import std from "std";
import { SchemaReader, SchemaWriter, END_OF_MESSAGE } from "cap:binary_schema";

fn encodeDecodeDemo(alloc: std.mem.Allocator)! {
    let w = SchemaWriter.init(alloc);
    w.writeFieldId(1)?;
    w.writeString("hello-cp")?;
    w.writeFieldId(2)?;
    w.writeU32(42)?;
    w.endMessage()?;

    let r = SchemaReader.init(w.bytes());
    while (!r.isEof()) {
        const fid = r.readFieldId()?;
        if (fid == END_OF_MESSAGE) break;
        if (fid == 1) { * = r.readString()?; }
        else if (fid == 2) { * = r.readU32()?; }
    }
}
```
