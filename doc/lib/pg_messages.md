# `cap:pg_messages` — 纯 CP PostgreSQL v3 后端控制与诊断消息零拷贝解码器

## 概述

`cap:pg_messages` 吸收自 Bun `src/sql/postgres/protocol/` 9 个核心协议模块，纯 CP 零外部依赖、零堆分配（所有字符串切片直接指向输入负载），提供 `ErrorResponse`/`NoticeResponse` 18 字段诊断消息、`BackendKeyData`、`ParameterStatus`、`ReadyForQuery`/`TransactionStatus`、`NotificationResponse` 与 `CommandComplete`（含影响行数解析）的完整解码能力。

## 核心类型与 API

- **报文类型常量**：`TAG_ERROR_RESPONSE` (`'E'`) / `TAG_NOTICE_RESPONSE` (`'N'`) / `TAG_BACKEND_KEY_DATA` (`'K'`) / `TAG_PARAMETER_STATUS` (`'S'`) / `TAG_READY_FOR_QUERY` (`'Z'`) / `TAG_NOTIFICATION_RESPONSE` (`'A'`) / `TAG_COMMAND_COMPLETE` (`'C'`)
- **错误集**：`PgMessageError`（`UnexpectedEof` / `InvalidMessageTag` / `InvalidMessageLength` / `InvalidNullTerminator` / `InvalidTransactionStatus`）
- **诊断消息**：`FieldType`（18 字段）、`SqlStateClass` + `classifySqlState(code)`、`DiagnosticMessage`（`sqlStateClass()` / `isFatal()`）、`decodeErrorResponse(data)` / `decodeNoticeResponse(data)`
- **控制与通知消息**：
  - `BackendKeyData` + `decodeBackendKeyData(data)`
  - `ParameterStatus` + `decodeParameterStatus(data)`
  - `TransactionStatus`（`idle`/`in_transaction`/`failed_transaction`）+ `ReadyForQuery` + `decodeReadyForQuery(data)`
  - `NotificationResponse` + `decodeNotificationResponse(data)`
  - `CommandComplete`（`command()` / `rowsAffected()`）+ `decodeCommandComplete(data)`

## 示例

```cp
import { decodeCommandComplete, decodeReadyForQuery } from "cap:pg_messages";

// CommandComplete frame: 'C' + len(14) + "SELECT 42\0"
const cc = decodeCommandComplete(frame_bytes)?;
// cc.command() == "SELECT", cc.rowsAffected().? == 42
```
