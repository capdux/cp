# `cap:binary_schema` — 纯 CP 紧凑二进制 Schema 流式编解码引擎

## 概述

`cap:binary_schema` 吸收自 Bun `src/analytics/schema.cp`，提供跨平台确定性小端二进制流式读取器 `SchemaReader`、可增长 RAII 堆写入器 `SchemaWriter`（`~init()` 自动释放）、零堆分配固定缓冲区写入器 `FixedSchemaWriter` 以及双 32 位紧凑整型 `CompactUint64`，支持整型、布尔、字节数组/字符串、泛型枚举合法性校验以及带字段 ID 的紧凑结构体记录编解码。

## 核心类型与 API

### 常量与错误集

- `END_OF_MESSAGE: u8 = 0`（字段 ID 结束哨兵）
- `SchemaError`：`UnexpectedEof` / `InvalidEnumValue` / `InvalidFieldId` / `BufferTooSmall` / `OutOfMemory`

### `SchemaReader`（零拷贝读取器）

- `SchemaReader.init(data: []const u8): SchemaReader`
- 状态查询：`remaining()`、`offset()`、`isEof()`
- 基本读取：`readBytes(count)`、`readByte()`、`readBool()`、`readI8()`、`readU16()`、`readI16()`、`readU32()`、`readI32()`、`readU64()`、`readI64()`、`readInt(T)`
- 长度前缀与结构化读取：`readByteArray()`、`readString()`、`readFieldId()`、`readEnum(Enum)`（`comptime for` 校验枚举成员）

### `SchemaWriter`（RAII 可增长写入器）

- `SchemaWriter.init(allocator)` / `SchemaWriter.initCapacity(allocator, initial_cap)`
- 查询与重置：`bytes()`、`written()`、`reset()`
- 写入方法：`writeBytes`、`writeByte`、`writeBool`、`writeI8`/`writeU16`/`writeI16`/`writeU32`/`writeI32`/`writeU64`/`writeI64`/`writeInt(T, v)`、`writeByteArray`、`writeString`、`writeFieldId`、`endMessage`、`writeEnum(Enum, val)`
- 自动析构：`~init()` 自动释放堆缓冲

### `FixedSchemaWriter` 与 `CompactUint64`

- `FixedSchemaWriter.init(buf: []u8)`：栈上零堆分配写入器，越界返回 `err.BufferTooSmall`
- `CompactUint64`：`fromU64(v)`、`toU64()`、`encode(*SchemaWriter)`、`decode(*SchemaReader)`

## 示例

```cp
import { SchemaReader, SchemaWriter, END_OF_MESSAGE } from "cap:binary_schema";

let w = SchemaWriter.init(alloc);
w.writeFieldId(1)?;
w.writeString("hello-cp")?;
w.writeFieldId(2)?;
w.writeU32(42)?;
w.endMessage()?;

let r = SchemaReader.init(w.bytes());
// 循环读取 field_id 直到 END_OF_MESSAGE (0)
```
