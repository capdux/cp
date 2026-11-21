# `cap:pg_wire_types` — 纯 CP PostgreSQL v3 线协议核心类型与编解码器

## 概述

`cap:pg_wire_types` 从 Bun `src/sql/postgres/protocol/FieldDescription.cp` 与 `StartupMessage.cp` 吸收并纯 CP 化，提供 PostgreSQL Wire Protocol v3.0 的核心枚举、大端整数编解码、`MessageHeader` 帧头编解码、`FieldDescription` 列元数据编解码，以及 `StartupMessage` 启动包序列化/反序列化，内置 RAII `~init()` 内存管理。

## 核心类型与 API

### 协议常量与错误集

- `PROTOCOL_VERSION_3_0: u32 = 196608`
- `SSL_REQUEST_CODE: u32 = 80877103`
- `CANCEL_REQUEST_CODE: u32 = 80877102`
- `PgWireError`：`UnexpectedEof` / `InvalidFormatCode` / `InvalidProtocolVersion` / `InvalidMessageLength` / `InvalidNullTerminator` / `InvalidMessageTag` / `EmptyUser` / `BufferTooSmall` / `OutOfMemory`

### 枚举

- `FormatCode`（`i16`）：`text = 0`、`binary = 1`，附 `isText()`、`isBinary()`、`toInt()` 与 `formatCodeFromInt(raw)`。
- `MessageTag`（`u8`）：PostgreSQL v3 全部标准前后端报文类型字节（`Authentication` / `RowDescription` / `DataRow` / `ReadyForQuery` / `Query` / `Parse` / `Bind` / `Terminate` 等），附 `toByte()`、`isFrontend()`、`isBackend()` 与 `messageTagFromByte(b)`。

### 大端编解码与帧头

- `readU16BE` / `readI16BE` / `readU32BE` / `readI32BE`
- `writeU16BE` / `writeI16BE` / `writeU32BE` / `writeI32BE`
- `MessageHeader` + `encodeMessageHeader(buf, tag, body_len)` / `decodeMessageHeader(data)`

### `FieldDescription` 与 `StartupMessage`

- `WireBuffer`：RAII 自动释放字节缓冲区（`slice()` / `len()` / `~init()`）。
- `FieldDescription` + `encodeFieldDescription(alloc, field)` / `decodeFieldDescription(data)`。
- `StartupParam` / `StartupMessage` / `DecodedStartupMessage` + `encodeStartupMessage(alloc, msg)` / `decodeStartupMessage(alloc, data)`。

## 示例

```cp
import {
    FormatCode,
    FieldDescription,
    StartupMessage,
    encodeFieldDescription,
    decodeFieldDescription,
    encodeStartupMessage,
    decodeStartupMessage,
} from "cap:pg_wire_types";

let startup_buf = encodeStartupMessage(alloc, StartupMessage{
    user: "postgres",
    database: "appdb",
    application_name: "cp-agent",
})?;
let decoded = decodeStartupMessage(alloc, startup_buf.slice())?;
// decoded.getParam("user") == "postgres"
// decoded.getParam("client_encoding") == "UTF8"
```
