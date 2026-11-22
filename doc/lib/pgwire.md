# `cap:pgwire` / `cap.pgwire` — PostgreSQL v3.0 Wire Protocol 标准库

## 1. 概述

`cap:pgwire` 是纯 CP 实现的工业级 PostgreSQL v3.0 Wire Protocol 二进制报文编解码与增量流式拆包标准库。
提供完整的 Frontend（客户端请求）与 Backend（服务端响应）报文建模、精确的大端序二进制编解码器、支持任意半包/黏包切片的增量流式解析器（含 `max_packet_bytes` 资源耗尽防护），以及标准 PostgreSQL MD5 密码认证辅助函数。

---

## 2. 核心类型

- **`PgError`**：协议解析与边界校验错误集合（`InvalidPacket`, `PacketTooLarge`, `Incomplete`, `UnexpectedEof`, `InvalidTag`, `OutOfMemory`）。
- **`PgTxStatus`**：事务状态枚举（`Idle = 'I'`, `InTransaction = 'T'`, `FailedTransaction = 'E'`）。
- **`PgAuthKind`**：认证请求联合体（`ok`, `kerberos_v5`, `cleartext_password`, `md5_password: [4]u8`, `scm_credential`, `gss`, `sspi`, `sasl`, `sasl_continue`, `sasl_final`, `unknown`）。
- **`PgColumnDesc`**：`RowDescription` 列元数据（`name`, `table_oid`, `column_attr`, `type_oid`, `type_size`, `type_modifier`, `format_code`）。
- **`PgFieldError`**：`ErrorResponse` / `NoticeResponse` 多字段结构（`severity`, `code`, `message`, `detail`, `hint`）。
- **`PgFrontendMsg`**：客户端报文联合体（`startup`, `ssl_request`, `query`, `parse`, `bind`, `execute`, `sync`, `terminate`, `password`, `unknown`）。
- **`PgBackendMsg`**：服务端报文联合体（`authentication`, `parameter_status`, `backend_key_data`, `ready_for_query`, `row_description`, `data_row`, `command_complete`, `empty_query_response`, `parse_complete`, `bind_complete`, `error_response`, `notice_response`, `unknown`）。
- **`PgBackendStreamDecoder` / `PgFrontendStreamDecoder`**：增量流式拆包器，支持任意 1..N 字节喂入与严格包长校验。

---

## 3. API 速查

| 符号 | 签名 / 说明 |
| :--- | :--- |
| `encodeFrontendMsg` | `(alloc, msg: PgFrontendMsg): ![]u8` — 编码客户端报文为大端序二进制帧 |
| `encodeBackendMsg` | `(alloc, msg: PgBackendMsg): ![]u8` — 编码服务端报文为大端序二进制帧 |
| `decodeFrontendPayload` | `(alloc, tag_opt: u8?, payload: []const u8): !PgFrontendMsg` — 解码客户端报文载荷 |
| `decodeBackendPayload` | `(alloc, tag: u8, payload: []const u8): !PgBackendMsg` — 解码服务端报文载荷 |
| `PgBackendStreamDecoder.init` | `(alloc, max_packet_bytes: usize): PgBackendStreamDecoder` |
| `PgBackendStreamDecoder.feed` | `(*_, chunk: []const u8): !void` — 喂入任意长度字节切片 |
| `PgBackendStreamDecoder.next` | `(*_): !PgBackendMsg?` — 拆出下一条完整报文，半包返回 `null` |
| `PgFrontendStreamDecoder.init` | `(alloc, max_packet_bytes: usize, expect_startup: bool): PgFrontendStreamDecoder` |
| `PgFrontendStreamDecoder.feed` | `(*_, chunk: []const u8): !void` — 喂入任意长度字节切片 |
| `PgFrontendStreamDecoder.next` | `(*_): !PgFrontendMsg?` — 拆出下一条完整报文（自动处理 Startup/SSLRequest 无 Tag 帧头） |
| `computePgMd5Password` | `(alloc, user, password, salt: [4]u8): ![]u8` — 计算 `"md5" + md5(md5(password + user) + salt4)` |
| `PgCommandTag.parse` | `(tag: []const u8): PgCommandTag` — 零堆分配解析 `CommandComplete` 命令标签（支持 `INSERT <oid> <rows>`、`DELETE`、`UPDATE`、`MERGE`、`SELECT`、`MOVE`、`FETCH`、`COPY` 与 `.other` 安全回退） |
| `PgCommandTag.format` | `(out: []u8): usize!` — 格式化回标准 PostgreSQL `CommandComplete` 命令标签字符串 |
| `PgCommandTag.rowCount` | `(): u64?` — 返回 8 类标准命令影响/返回行数，`.other` 返回 `null` |
| `PgCommandTag.isDml` / `isQueryOrCursor` | `(): bool` — 判断是否为 DML（`INSERT`/`DELETE`/`UPDATE`/`MERGE`）或查询/游标（`SELECT`/`MOVE`/`FETCH`）命令 |

### `CommandComplete` 命令标签解析（`commandTag.cp`）

> 吸收自 `/data/dev/bun/src/sql/postgres/CommandTag.cp`，零堆分配解析并格式化 PostgreSQL v3.0 `CommandComplete` 报文携带的命令标签，严格校验 64 位无符号整数溢出与异常格式。

- 核心导出：`PgCommandTagError`、`PgCommandKind`、`PgCommandTag`。

```typescript
import { PgCommandTag, PgCommandKind } from "cap:pgwire";

const tag = PgCommandTag.parse("INSERT 0 42");
// tag.kind == PgCommandKind.Insert, tag.oid == 0, tag.rows == 42, tag.isDml() == true
let buf: [64]u8 = [0; 64];
const n = tag.format(buf[0..])?;
// buf[0..n] == "INSERT 0 42"
```

### `NegotiateProtocolVersion`（`'v'`）协议协商报文编解码（`negotiateProtocol.cp`）

> 吸收自 `/data/dev/bun/src/sql/postgres/protocol/NegotiateProtocolVersion.cp`，零堆分配实现 PostgreSQL v3.0 协议版本协商响应报文（Tag `'v' = 0x76`）的大端序编解码、版本拆解与异常边界防护。

- 核心导出：`PG_TAG_NEGOTIATE_PROTOCOL_VERSION` (`'v'`)、`PG_NEGOTIATE_MAX_OPTIONS` (`32`)、`PgNegotiateError`、`PgNegotiateProtocolVersion`（`init` / `addOption` / `count` / `optionAt` / `containsOption` / `majorVersion` / `minorVersion` / `encodedPayloadLen` / `encodedFrameLen` / `encodePayload` / `encodeFrame` / `decodePayload` / `decodeFrame` / `eql`）。

```typescript
import { PgNegotiateProtocolVersion } from "cap:pgwire";

let msg = PgNegotiateProtocolVersion.init(0);
* = msg.addOption("_pq_.foo")?;
let frame: [64]u8 = [0; 64];
const n = msg.encodeFrame(frame[0..])?;
const decoded = PgNegotiateProtocolVersion.decodeFrame(frame[0..n])?;
```

### `ErrorResponse` / `NoticeResponse` 18 字段诊断消息编解码（`fieldMessage.cp`）

> 吸收自 `/data/dev/bun/src/sql/postgres/protocol/FieldMessage.cp`，零堆分配完整支持 PostgreSQL v3.0 协议全部 18 种诊断字段（`S`/`V`/`C`/`M`/`D`/`H`/`P`/`p`/`q`/`W`/`s`/`t`/`c`/`d`/`n`/`F`/`L`/`R`）、SQLSTATE 分类提取与可重试事务冲突判定（`40001` / `40P01`）。

- 核心导出：`PG_MAX_DIAGNOSTIC_FIELDS` (`24`)、`PgFieldMessageError`、`PgDiagnosticFieldType`、`parseDiagnosticFieldType`、`diagnosticFieldName`、`PgDiagnosticField`、`PgDiagnosticFields`（`init` / `add` / `count` / `at` / `get` / `severity` / `code` / `message` / `detail` / `hint` / `schema` / `table` / `column` / `constraint` / `sqlstateClass` / `isRetryable` / `encodedLen` / `encodePayload` / `decodePayload` / `decodePayloadStrict` / `eql`）。

```typescript
import { PgDiagnosticFields, PgDiagnosticFieldType } from "cap:pgwire";

let fields = PgDiagnosticFields.init();
* = fields.add(PgDiagnosticFieldType.Severity, "ERROR")?;
* = fields.add(PgDiagnosticFieldType.Code, "23505")?;
* = fields.add(PgDiagnosticFieldType.Message, "duplicate key value violates unique constraint")?;
* = fields.add(PgDiagnosticFieldType.Constraint, "users_email_key")?;
// fields.sqlstateClass() == "23", fields.constraint() == "users_email_key"
```

### `NotificationResponse`（`'A'`）LISTEN/NOTIFY 异步通知编解码（`notificationResponse.cp`）

> 吸收自 `/data/dev/bun/src/sql/postgres/protocol/NotificationResponse.cp`（参考 `/data/dev/bun/docs/runtime/sql.mdx` PostgreSQL 二进制协议与连接管线最佳实践），零堆分配实现 PostgreSQL v3.0 `NotificationResponse` 报文（Tag `'A' = 0x41`）的零拷贝编解码与异常边界防护。

- 核心导出：`PG_TAG_NOTIFICATION_RESPONSE`（`'A'`）、`PG_NOTIFY_DEFAULT_MAX_PAYLOAD`（`8000`）、`PgNotificationError`（`InvalidTag`/`InvalidLength`/`Truncated`/`MissingNulTerminator`/`EmbeddedNul`/`EmptyChannel`/`TrailingBytes`/`BufferTooSmall`）、`PgNotificationResponse`（`init`/`hasPayload`/`isChannel`/`isWithinDefaultPayloadLimit`/`encodedPayloadLen`/`encodedFrameLen`/`validate`/`encodePayload`/`encodeFrame`/`decodePayload`/`decodeFrame`/`eql`）；
- **安全与实战最佳实践**：
  - 解码切片直接零拷贝借用输入报文缓冲区，严格校验 `Int32` 大端长度、频道非空、双 NUL 终结符与零尾随字节；
  - 编码前自动拦截空频道名与内嵌 `0x00` 字节（防止 C-string 协议截断注入），并可通过 `isWithinDefaultPayloadLimit()` 校验是否遵守 PostgreSQL 默认 8000 字节 `NOTIFY` 负载上限。

```cp
import { PgNotificationResponse } from "cap:pgwire";

const note = PgNotificationResponse.init(4219, "orders_events", "{\"id\":99}");
let frame: [128]u8 = [0; 128];
const n = note.encodeFrame(frame[0..])?;
const decoded = PgNotificationResponse.decodeFrame(frame[0..n])?;
* = decoded.isChannel("orders_events"); // true
```

---

## 4. 示例

```cp
import {
    PgFrontendMsg,
    PgFrontendStreamDecoder,
    PgKv,
    DEFAULT_MAX_PACKET_BYTES,
    encodeFrontendMsg,
    computePgMd5Password,
} from "cap:pgwire";
import cap from "cap";

// 1. 编码 StartupMessage 并流式解码
let kvs: [2]PgKv = [
    PgKv{ .key = "user", .value = "postgres" },
    PgKv{ .key = "database", .value = "app" },
];
const frame = try encodeFrontendMsg(alloc, PgFrontendMsg{ .startup = &kvs });
defer alloc.free(frame);

let decoder = PgFrontendStreamDecoder.init(alloc, DEFAULT_MAX_PACKET_BYTES, true);
defer decoder.deinit();
try decoder.feed(frame);
if (msg := try decoder.next()) {
    let owned = msg;
    defer owned.deinit(alloc);
}

// 2. 通过 cap.pgwire 动态挂载计算 MD5 密码
const salt: [4]u8 = [1, 2, 3, 4];
const md5_pw = try cap.pgwire.computePgMd5Password(alloc, "postgres", "secret", salt);
defer alloc.free(md5_pw);
```
