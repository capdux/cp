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

### `Close`（`'C'`）/ `Describe`（`'D'`）与 `CloseComplete`（`'3'`）扩展查询生命周期编解码（`closeDescribe.cp`）

> 吸收自 `/data/dev/bun/src/sql/postgres/protocol/Close.cp`（参考 `/data/dev/bun/docs/runtime/sql.mdx` PostgreSQL Extended Query 与预处理语句缓存最佳实践），零堆分配实现 `Close`、`Describe` 与 `CloseComplete` 帧编解码及 `PgTargetRef`（`'S'` 预处理语句 / `'P'` 游标 Portal）。

- 核心导出：`PG_TAG_CLOSE`（`'C'`）、`PG_TAG_DESCRIBE`（`'D'`）、`PG_TAG_CLOSE_COMPLETE`（`'3'`）、`PG_TARGET_PREPARED_STATEMENT`（`'S'`）、`PG_TARGET_PORTAL`（`'P'`）、`PgCloseDescribeError`、`PgTargetKind`、`PgTargetRef`、`PgCloseMsg`（含 `encodeCloseCompleteFrame` / `decodeCloseCompleteFrame`）、`PgDescribeMsg`；
- **实战最佳实践**：
  - 复用预处理语句缓存淘汰旧语句时，在管线中发送 `PgCloseMsg.statement(name)` 并等待服务端 `CloseComplete`（`'3'`）确认，避免连接侧 Prepared Statement 泄漏；
  - 一次性参数化查询可使用未命名目标 `PgTargetRef.unnamedStatement()` / `PgTargetRef.unnamedPortal()`（空名称 + 单个 NUL 终结符），无需额外 `Close` 往返即可被下一次 `Parse`/`Bind` 自动覆写。

```cp
import { PgCloseMsg, PgDescribeMsg } from "cap:pgwire";

let buf: [64]u8 = [0; 64];
const n1 = PgDescribeMsg.statement("stmt_users_by_id").encodeFrame(buf[0..])?;
const desc = PgDescribeMsg.decodeFrame(buf[0..n1])?;
const n2 = PgCloseMsg.portal("cursor_1").encodeFrame(buf[0..])?;
const cls = PgCloseMsg.decodeFrame(buf[0..n2])?;
* = desc.target.name;
* = cls.target.name;
```

### `COPY` 流式数据帧编解码（`CopyData` `'d'` / `CopyDone` `'c'` / `CopyFail` `'f'`，`copyFrames.cp`）

> 吸收自 `/data/dev/bun/src/sql/postgres/protocol/CopyFail.cp` 与 `CopyData.cp`（参考 `/data/dev/bun/docs/runtime/sql.mdx` PostgreSQL 批量流式写入实践），零堆分配实现 PostgreSQL v3.0 `COPY FROM STDIN` / `COPY TO STDOUT` 全套流帧编解码与异常中止协议。

- 核心导出：`PG_TAG_COPY_DATA`（`'d'`）、`PG_TAG_COPY_DONE`（`'c'`）、`PG_TAG_COPY_FAIL`（`'f'`）、`PG_COPY_DONE_FRAME_LEN`（`5`）、`PgCopyError`、`PgCopyData`（支持含 `0x00` 的任意二进制/文本块）、`PgCopyDone`（固定 5 字节完结帧）、`PgCopyFail`（携带 NUL 终结中止原因，自动拦截内嵌 NUL 与畸形帧）；
- **实战最佳实践**：
  - 客户端在 `COPY FROM STDIN` 过程中如遇本地数据源错误，应发送 `PgCopyFail.init("reason")` 而非直接断开 TCP，使服务端回滚 `COPY` 并返回规范 `ErrorResponse` + `ReadyForQuery`，保持连接池会话可复用。

```cp
import { PgCopyData, PgCopyDone, PgCopyFail } from "cap:pgwire";

let buf: [64]u8 = [0; 64];
const n_data = PgCopyData.init("1\talice\n").encodeFrame(buf[0..])?;
const d = PgCopyData.decodeFrame(buf[0..n_data])?;
const n_done = PgCopyDone.encodeFrame(buf[0..])?;
* = PgCopyDone.decodeFrame(buf[0..n_done])?;
const n_fail = PgCopyFail.init("upstream csv parse error").encodeFrame(buf[0..])?;
* = d.data; * = n_fail;
```

### `NoticeResponse`（`'N'`）异步诊断报文编解码（`noticeResponse.cp`）

吸收自 `/data/dev/bun/src/sql/postgres/protocol/NoticeResponse.cp`，零堆分配编解码 PostgreSQL v3.0 Backend `NoticeResponse`（`'N' = 0x4E`）异步诊断帧，直接复用 `PgDiagnosticFields` 18 字段容器：

- **常量**：`PG_TAG_NOTICE_RESPONSE` (`'N'`)、`PG_NOTICE_HEADER_LEN` (`5`)。
- **严重级别**：`PgNoticeSeverity`（`Notice` / `Warning` / `Info` / `Log` / `Debug` / `Unknown`）及 `parseNoticeSeverity(s)`（自动识别 `NOTICE` / `WARNING` / `INFO` / `LOG` / `DEBUG`..`DEBUG5`）。
- **编解码**：
  - `PgNoticeResponse.init()` / `.fromFields(fields)` / `.addField(field_type, value)`（拒绝内嵌 `0x00` 字节）；
  - `.encodeFrame(out)` / `.encodedLen()` / `.payloadLen()`；
  - `PgNoticeResponse.decodeFrame(frame)` / `.decodeFrameStrict(frame)` / `.decodePayload(payload)` / `.decodePayloadStrict(payload)`（兼容最小空载荷帧 `len == 4`，宽容模式自动跳过未来扩展字段，严格模式校验 18 种已知字段）。
- **便捷读取**：`.severity()`（优先 `'V'` 非本地化严重级别，回退 `'S'`）、`.severityLevel()`、`.isWarning()`、`.isNotice()`、`.code()`、`.sqlstateClass()`、`.message()`、`.detail()`、`.hint()`、`.schema()`、`.table()`、`.column()`、`.constraint()`、`.whereContext()`、`.file()`、`.line()`、`.routine()`。
- **生产实践**：
  - PostgreSQL 可在任意查询阶段异步推送 `NoticeResponse`（如 `RAISE NOTICE`、`CREATE TABLE IF NOT EXISTS` 跳过提示、连接池告警），客户端事件循环收到 `'N'` 帧时应消费并转发给日志回调，而**不应中断当前查询状态机**。

```cp
import {
    PgDiagnosticFieldType,
    PgNoticeResponse,
    PgNoticeSeverity,
} from "cap:pgwire";

let n = PgNoticeResponse.init();
* = n.addField(PgDiagnosticFieldType.Severity, "NOTICE")?;
* = n.addField(PgDiagnosticFieldType.LocalizedSeverity, "NOTICE")?;
* = n.addField(PgDiagnosticFieldType.Code, "42P07")?;
* = n.addField(PgDiagnosticFieldType.Message, "relation \"users\" already exists, skipping")?;

let buf: [256]u8 = [0; 256];
const written = n.encodeFrame(buf[0..])?;
const decoded = PgNoticeResponse.decodeFrame(buf[0..written])?;
* = decoded.severityLevel() == PgNoticeSeverity.Notice;
```

### Frontend 认证响应帧编解码（`PasswordMessage` / `SASLInitialResponse` / `SASLResponse`，`saslFrames.cp`）

吸收自 `/data/dev/bun/src/sql/postgres/protocol/SASLInitialResponse.cp`，零堆分配编解码 PostgreSQL v3.0 前端 `'p'` 认证响应三件套：

- **常量**：`PG_TAG_PASSWORD_MESSAGE` (`'p'`)、`PG_SASL_SCRAM_SHA256` (`"SCRAM-SHA-256"`)、`PG_SASL_SCRAM_SHA256_PLUS` (`"SCRAM-SHA-256-PLUS"`)、`PG_SASL_NO_INITIAL_DATA` (`-1`)。
- **`PgPasswordMessage`**：明文或 MD5 密码帧（`['p'][len][password\0]`），提供 `init` / `encodedLen` / `encodeFrame` / `decodeFrame` / `isMd5Prefixed` / `eql`（拒绝内嵌 NUL 字节）。
- **`PgSaslInitialResponse`**：SASL 初始响应帧（`['p'][len][mechanism\0][data_len: i32 BE][initial_data?]`），支持 `initial_data == null`（规范编码为 `-1` / `0xFFFFFFFF`），提供 `init` / `scramSha256` / `encodedLen` / `encodeFrame` / `decodeFrame` / `isScramSha256` / `isScramSha256Plus` / `eql`。
- **`PgSaslResponse`**：SASL 后续挑战响应帧（`['p'][len][data]`），提供 `init` / `encodedLen` / `encodeFrame` / `decodeFrame` / `eql`。

```cp
import {
    PgPasswordMessage,
    PgSaslInitialResponse,
    PgSaslResponse,
} from "cap:pgwire";

let buf: [256]u8 = [0; 256];
const n1 = PgSaslInitialResponse.scramSha256("n,,n=,r=rOprNGfwEbeRWgbNEkqO").encodeFrame(buf[0..])?;
const init_msg = PgSaslInitialResponse.decodeFrame(buf[0..n1])?;
const n2 = PgSaslResponse.init("c=biws,r=rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0,p=dHzbZapWIk4jUhN+Ute9ytag9zjfMHgsqmmiz7AndVQ=").encodeFrame(buf[0..])?;
const resp_msg = PgSaslResponse.decodeFrame(buf[0..n2])?;
const n3 = PgPasswordMessage.init("md50123456789abcdef0123456789abcdef").encodeFrame(buf[0..])?;
const pw_msg = PgPasswordMessage.decodeFrame(buf[0..n3])?;
* = init_msg.isScramSha256(); * = resp_msg.data; * = pw_msg.isMd5Prefixed();
```

### 会话控制与查询取消帧编解码（`BackendKeyData` / `CancelRequest` / `ParameterStatus` / `ReadyForQuery`，`sessionFrames.cp`）

吸收自 `/data/dev/bun/src/sql/postgres/protocol/BackendKeyData.cp`，零堆分配编解码 PostgreSQL v3.0 会话生命周期 4 大核心帧：

- **常量**：`PG_TAG_BACKEND_KEY_DATA` (`'K'`)、`PG_TAG_PARAMETER_STATUS` (`'S'`)、`PG_TAG_READY_FOR_QUERY` (`'Z'`)、`PG_CANCEL_REQUEST_CODE` (`80877102`)、`PG_BACKEND_KEY_FRAME_LEN` (`13`)、`PG_CANCEL_REQUEST_FRAME_LEN` (`16`)、`PG_READY_FOR_QUERY_FRAME_LEN` (`6`)。
- **`PgBackendKeyFrame` (`'K'`) & `PgCancelRequestFrame`**：编码/解码后端进程号与取消密钥，`backend_key.toCancelRequest()` 直接构造 16 字节独立短连接 `CancelRequest` 帧（无 tag 字节），用于生产级慢查询/超时取消。
- **`PgParameterStatusFrame` (`'S'`)**：零拷贝解析运行时参数键值对（如 `server_version`、`client_encoding`、`server_encoding`、`TimeZone`、`standard_conforming_strings`），内置常用参数识别器与内嵌 NUL / 空键名防护。
- **`PgReadyForQueryFrame` (`'Z'`) & `PgTransactionState`**：解析 `'I'`（`Idle`）、`'T'`（`InTransaction`）、`'E'`（`FailedTransaction`）状态，提供 `.isIdle()` / `.inTransaction()` / `.isFailedTransaction()` / `.canSendQuery()` / `.needsRollback()` 辅助连接池判定会话复用。

```cp
import {
    PgBackendKeyFrame,
    PgCancelRequestFrame,
    PgParameterStatusFrame,
    PgReadyForQueryFrame,
    PgTransactionState,
} from "cap:pgwire";

let buf: [128]u8 = [0; 128];
const n_key = PgBackendKeyFrame.init(42195, 0xDEADBEEF).encodeFrame(buf[0..])?;
const key = PgBackendKeyFrame.decodeFrame(buf[0..n_key])?;
const n_cancel = key.toCancelRequest().encodeFrame(buf[0..])?;
const cancel = PgCancelRequestFrame.decodeFrame(buf[0..n_cancel])?;

const n_ps = PgParameterStatusFrame.init("server_version", "16.4").encodeFrame(buf[0..])?;
const ps = PgParameterStatusFrame.decodeFrame(buf[0..n_ps])?;

const n_rfq = PgReadyForQueryFrame.init(PgTransactionState.Idle).encodeFrame(buf[0..])?;
const rfq = PgReadyForQueryFrame.decodeFrame(buf[0..n_rfq])?;
* = cancel.process_id; * = ps.isServerVersion(); * = rfq.isIdle();
```

### 连接握手与传输升级帧编解码（`StartupMessage` / `SSLRequest` / `GSSENCRequest`，`startupFrame.cp`）

吸收自 `/data/dev/bun/src/sql/postgres/protocol/StartupMessage.cp`，零堆分配编解码 PostgreSQL v3.0 连接启动与 TLS/GSSAPI 升级握手帧：

- **常量**：`PG_STARTUP_PROTOCOL_V3` (`196608`)、`PG_STARTUP_SSL_REQUEST_CODE` (`80877103`)、`PG_STARTUP_GSSENC_REQUEST_CODE` (`80877104`)、`PG_STARTUP_UPGRADE_FRAME_LEN` (`8`)、`PG_STARTUP_MAX_PARAMS` (`16`)。
- **`PgStartupMessage`**：固定容量 16 个启动参数键值对（`PgStartupParam`），支持：
  - `PgStartupMessage.withDefaults(user, database)`（对齐 Bun 语义：`database` 为空时默认回退为 `user`，自动附加 `client_encoding = "UTF8"`）；
  - `.addParam(key, value)` / `.get(key)` / `.user()` / `.database()` / `.clientEncoding()` / `.applicationName()` / `.protocolMajor()` / `.protocolMinor()`；
  - `.encodedLen()` / `.encodeFrame(out)`（校验必填 `user` 与无内嵌 NUL）/ `PgStartupMessage.decodeFrame(frame)`（校验主版本号 3、成对 NUL 终止及尾部 `0x00`）。
- **`PgTlsUpgradeRequest` (`PgUpgradeKind.Ssl` / `PgUpgradeKind.GssEnc`)**：8 字节传输层升级请求帧编解码，内置服务端单字节响应判定 `.isAcceptedByte(resp)`（`'S'` / `'G'`）与 `PgTlsUpgradeRequest.isRejectedByte(resp)`（`'N'`）。

```cp
import {
    PgStartupMessage,
    PgTlsUpgradeRequest,
} from "cap:pgwire";

let buf: [256]u8 = [0; 256];
const ssl = PgTlsUpgradeRequest.ssl();
const n_ssl = ssl.encodeFrame(buf[0..])?;
* = ssl.isAcceptedByte('S');

let startup = PgStartupMessage.withDefaults("postgres", "app_prod")?;
* = startup.addParam("application_name", "cp_worker")?;
const n_start = startup.encodeFrame(buf[0..])?;
const decoded = PgStartupMessage.decodeFrame(buf[0..n_start])?;
* = n_ssl; * = decoded.database();
```

### `DataRow`（`'D'`）零拷贝行视图与流式编码器（`dataRow.cp`）

吸收自 `/data/dev/bun/src/sql/postgres/protocol/DataRow.cp`，零堆分配编解码 PostgreSQL v3.0 结果集热路径 `DataRow`（`'D' = 0x44`）报文：

- **常量**：`PG_TAG_DATA_ROW` (`'D'`)、`PG_DATA_ROW_NULL_LEN` (`-1` / `0xFFFFFFFF`)、`PG_DATA_ROW_HEADER_LEN` (`7`)。
- **`PgDataRowView` & `PgDataRowIter` & `PgDataRowCell`**：
  - `PgDataRowView.decodeFrame(frame)` / `.decodePayload(payload)` 在构造时单遍校验全部列长度与帧边界，精确区分 SQL `NULL`（`value == null`）与空字节切片 `""`（`value.len == 0`）；
  - `.iter()` 返回零拷贝顺序迭代器（`.next()` / `.remaining()`）；
  - `.columnAt(idx)` / `.columnCount()` / `.isEmpty()` / `.nullCount()` / `.nonNullCount()`。
- **`PgDataRowEncoder`**：写入外部 `[]u8` 缓冲区的流式帧编码器，提供 `init(buf)` / `writeNull()` / `writeBytes(data)` / `writeCell(opt)` / `columnCount()` / `writtenLen()` / `finish()`（自动回填 `'D'` 帧头、4B 总长度与 2B 列数）。

```cp
import { PgDataRowEncoder, PgDataRowView } from "cap:pgwire";

let buf: [256]u8 = [0; 256];
let enc = PgDataRowEncoder.init(buf[0..])?;
* = enc.writeBytes("42")?;
* = enc.writeBytes("alice")?;
* = enc.writeNull()?;
* = enc.writeBytes("")?;
const frame_len = enc.finish()?;

const row = PgDataRowView.decodeFrame(buf[0..frame_len])?;
let it = row.iter();
while (cell := it.next()) {
    * = cell.isNull();
}
```

### `RowDescription`（`'T'`）零拷贝列元数据视图与流式编码器（`rowDesc.cp`）

吸收自 `/data/dev/bun/src/sql/postgres/protocol/RowDescription.cp`，零堆分配编解码 PostgreSQL v3.0 结果集列元数据 `RowDescription`（`'T' = 0x54`）报文，与 `DataRow`（`'D'`）构成完整零拷贝结果集消费链路：

- **常量**：`PG_TAG_ROW_DESCRIPTION` (`'T'`)、`PG_ROW_DESC_HEADER_LEN` (`7`)、`PG_ROW_DESC_FIELD_META_LEN` (`18`)。
- **格式码**：`PgFormatCode`（`Text = 0` / `Binary = 1`）与 `parseFormatCode(raw)`。
- **`PgRowDescView` & `PgRowDescIter` & `PgRowFieldDesc`**：
  - `PgRowDescView.decodeFrame(frame)` / `.decodePayload(payload)` 在构造时单遍校验全部字段的 NUL 终止、18B 元数据边界与格式码（仅允许 `0` 或 `1`）；
  - `.iter()` / `.fieldAt(idx)` / `.findByName(name)` / `.indexOfName(name)` / `.fieldCount()` / `.isEmpty()` / `.allText()` / `.allBinary()`；
  - `PgRowFieldDesc` 提供 `name`、`table_oid`、`column_attr`、`type_oid`、`type_size`、`type_modifier`、`format` 及 `.isBinary()` / `.isText()` / `.isVariableLength()`。
- **`PgRowDescEncoder`**：流式 `'T'` 帧编码器，提供 `init(buf)` / `writeField(field)` / `writeSimpleText(name, type_oid)` / `fieldCount()` / `writtenLen()` / `finish()`。

```cp
import { PgRowDescEncoder, PgRowDescView, PgRowFieldDesc, PgFormatCode } from "cap:pgwire";

let buf: [256]u8 = [0; 256];
let enc = PgRowDescEncoder.init(buf[0..])?;
* = enc.writeField(PgRowFieldDesc{
    name: "id",
    table_oid: 16384,
    column_attr: 1,
    type_oid: 23, // INT4
    type_size: 4,
    type_modifier: -1,
    format: PgFormatCode.Binary,
})?;
* = enc.writeSimpleText("email", 25)?; // TEXT
const n = enc.finish()?;

const desc = PgRowDescView.decodeFrame(buf[0..n])?;
* = desc.indexOfName("email");
```

### Extended Query `Parse`（`'P'`）与 5 字节控制信号帧编解码（`parseFrame.cp`）

吸收自 `/data/dev/bun/src/sql/postgres/protocol/Parse.cp`，零堆分配编解码 PostgreSQL v3.0 扩展查询协议的 `Parse`（`'P'`）语句预编译帧以及全部 7 种 5 字节无载荷控制信号帧：

- **常量**：`PG_TAG_PARSE` (`'P'`)、`PG_TAG_SYNC` (`'S'`)、`PG_TAG_FLUSH` (`'H'`)、`PG_TAG_PARSE_COMPLETE` (`'1'`)、`PG_TAG_BIND_COMPLETE` (`'2'`)、`PG_TAG_NO_DATA` (`'n'`)、`PG_TAG_EMPTY_QUERY` (`'I'`)、`PG_TAG_PORTAL_SUSPENDED` (`'s'`)、`PG_EMPTY_FRAME_LEN` (`5`)、`PG_PARSE_MAX_PARAM_OIDS` (`64`)。
- **`PgParseFrame`**：固定容量 64 个参数类型 OID，提供 `init(stmt_name, sql)` / `unnamed(sql)` / `addParamOid(oid)` / `isUnnamed()` / `paramCount()` / `paramOidAt(idx)` / `encodedLen()` / `encodeFrame(out)` / `decodeFrame(frame)` / `eql(other)`。
- **`PgEmptySignalFrame` & `PgEmptySignalKind`**：统一编解码 5 字节控制信号帧（`Sync` / `Flush` / `ParseComplete` / `BindComplete` / `NoData` / `EmptyQueryResponse` / `PortalSuspended`），提供 `init` / `encodeFrame` / `decodeAny` / `decodeFrame(frame, expected_kind)` / `parseEmptySignalKind`。

```cp
import { PgParseFrame, PgEmptySignalFrame, PgEmptySignalKind } from "cap:pgwire";

let buf: [256]u8 = [0; 256];
let p = PgParseFrame.init("stmt_user_by_id", "SELECT id, email FROM users WHERE id = $1");
* = p.addParamOid(23)?; // INT4
const n_p = p.encodeFrame(buf[0..])?;
const dec_p = PgParseFrame.decodeFrame(buf[0..n_p])?;

const n_sync = PgEmptySignalFrame.init(PgEmptySignalKind.Sync).encodeFrame(buf[0..])?;
const sync_sig = PgEmptySignalFrame.decodeFrame(buf[0..n_sync], PgEmptySignalKind.Sync)?;
* = dec_p.paramCount(); * = sync_sig.kind;
```

### Backend `Authentication`（`'R'`）全认证挑战与 SCRAM 解析（`authChallenge.cp`）

吸收自 `/data/dev/bun/src/sql/postgres/protocol/Authentication.cp`，零堆分配编解码 PostgreSQL v3.0 后端 `Authentication`（`'R' = 0x52`）全部 11 种认证子类型及未知扩展：

- **常量**：`PG_TAG_AUTHENTICATION` (`'R'`)、`PG_AUTH_HEADER_LEN` (`9`)、`PG_AUTH_CODE_OK` (`0`)、`PG_AUTH_CODE_KERBEROS_V5` (`2`)、`PG_AUTH_CODE_CLEARTEXT` (`3`)、`PG_AUTH_CODE_MD5` (`5`)、`PG_AUTH_CODE_SCM_CREDENTIAL` (`6`)、`PG_AUTH_CODE_GSS` (`7`)、`PG_AUTH_CODE_GSS_CONTINUE` (`8`)、`PG_AUTH_CODE_SSPI` (`9`)、`PG_AUTH_CODE_SASL` (`10`)、`PG_AUTH_CODE_SASL_CONTINUE` (`11`)、`PG_AUTH_CODE_SASL_FINAL` (`12`)、`PG_AUTH_MAX_SASL_MECHANISMS` (`8`)。
- **`PgAuthChallenge` & `PgAuthChallengeKind`**：支持 `ok()` / `cleartext()` / `md5(salt)` / `sasl(mechanisms)` / `saslContinue(server_first)` / `saslFinal(server_final)` / `gssContinue(token)`，以及 `encodeFrame(out)` / `decodeFrame(frame)` / `isOk()` / `requiresPassword()` / `isSaslStep()` / `parseServerFinal()`。
- **`PgSaslMechanismList`**：零拷贝解析 `AuthenticationSASL` 机制列表，提供 `count()` / `at(idx)` / `supports(mech)` / `supportsScramSha256()` / `supportsScramSha256Plus()`。
- **`PgScramServerFirst` & `PgScramServerFinal`**：零拷贝解析 SCRAM-SHA-256 `r=<nonce>,s=<salt_b64>,i=<iterations>`（带严格正整数迭代次数解析）与 `v=<verifier_b64>` / `e=<err_msg>`。

```cp
import { PgAuthChallenge, PgAuthChallengeKind } from "cap:pgwire";

let buf: [256]u8 = [0; 256];
const c = PgAuthChallenge.saslContinue("r=rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0,s=W22ZaJ0SNY7soEsUEjb6gQ==,i=4096")?;
const n = c.encodeFrame(buf[0..])?;
const dec = PgAuthChallenge.decodeFrame(buf[0..n])?;
* = dec.kind == PgAuthChallengeKind.SaslContinue;
* = dec.scram_first.iterations; // 4096
```

### `ParameterDescription`（`'t'`）预处理语句参数类型 OID 编解码（`paramDesc.cp`）

> 吸收自 `/data/dev/bun/src/sql/postgres/protocol/ParameterDescription.cp`，零堆分配实现 PostgreSQL v3.0 Backend `ParameterDescription`（Tag `'t' = 0x74`）报文的大端序零拷贝解码视图、OID 迭代器与栈/切片编码器。

- 核心导出：`PG_TAG_PARAMETER_DESCRIPTION`（`'t'`）、`PG_PARAM_DESC_HEADER_LEN`（`7`）、`PG_PARAM_DESC_MAX_OIDS`（`64`）、`PgParamDescError`（`InvalidTag`/`InvalidLength`/`NegativeCount`/`Truncated`/`TrailingBytes`/`BufferTooSmall`/`TooManyParams`/`IndexOutOfBounds`）、`PgParamDescIter`（`next`/`remaining`/`reset`）、`PgParamDescView`（`decodePayload`/`decodeFrame`/`isEmpty`/`oidAt`/`iter`/`copyOids`/`containsOid`/`hasUnspecifiedOid`）、`PgParamDescEncoder`（`init`/`addOid`/`payloadLen`/`frameLen`/`encodePayload`/`encodeFrame`/`encodeFrameFromSlice`）；
- **实战最佳实践**：
  - 在 Extended Query 管线中发送 `Parse` + `Describe('S', name)` 后，服务端先返回 `ParameterDescription`（`'t'`）列出各占位符 `$1..$N` 推断的 PostgreSQL Type OID，随后返回 `RowDescription`（`'T'`）或 `NoData`（`'n'`）；
  - `PgParamDescView` 直接零拷贝借用输入报文缓冲，支持任意参数个数；`encodeFrameFromSlice` 同样支持任意长度 OID 切片（上限受协议 `Int16` 32767 约束）。

```cp
import { PgParamDescEncoder, PgParamDescView } from "cap:pgwire";

let enc = PgParamDescEncoder.init();
* = enc.addOid(23)?;   // INT4
* = enc.addOid(25)?;   // TEXT
* = enc.addOid(2950)?; // UUID

let buf: [64]u8 = [0; 64];
const n = enc.encodeFrame(buf[0..])?;
const view = PgParamDescView.decodeFrame(buf[0..n])?;
const oid0 = view.oidAt(0)?; // 23
* = oid0;
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
