# `cap:pg` — 纯 CP PostgreSQL Wire Protocol v3 全栈协议与类型工具库

> 参考来源：`/data/dev/bun/src/sql/postgres/`, `/data/dev/bun/docs/runtime/sql.mdx` | 吸收日期：2026-11-22

## 概述

`cap:pg` 吸收自 Bun `src/sql/postgres/` 系列核心模块，将 PostgreSQL Wire Protocol v3 线协议基础类型、认证握手、COPY/SASL/协议协商、扩展查询构建器、后端控制与诊断消息、零拷贝帧读取器、内置 OID 类型目录与 SQLSTATE 错误码统一收敛至单一领域主包 `cap:pg`。纯 CP 零外部依赖，堆分配缓冲类型均内置 RAII `~init()` 零泄漏释放。

## 子模块拓扑

| 文件 | 职责 |
| :--- | :--- |
| `lib/pg/auth.cp` | 后端认证握手报文解码（`Ok`/`Cleartext`/`MD5`/`SASL`/`SASLContinue`/`SASLFinal`）与大端长度回填写入器 `PgWireWriter` |
| `lib/pg/copysasl.cp` | COPY 协议（`CopyInResponse`/`CopyOutResponse`/`CopyData`/`CopyDone`/`CopyFail`）、前端认证报文（`PasswordMessage`/`SASLInitialResponse`/`SASLResponse`）与 `NegotiateProtocolVersion` |
| `lib/pg/extquery.cp` | 扩展查询协议前端消息编码器 `ExtendedQueryBuilder`（`Parse`/`Bind`/`Describe`/`Execute`/`Close`/`Sync`/`Flush`） |
| `lib/pg/messages.cp` | 后端控制与诊断消息零拷贝解码（`ErrorResponse`/`NoticeResponse` 18 字段、`BackendKeyData`、`ParameterStatus`、`ReadyForQuery`、`NotificationResponse`、`CommandComplete`） |
| `lib/pg/oids.cp` | PostgreSQL 内置 45 项标量/数组 OID 目录（`PgOid`）、大类分类（`PgTypeCategory`）、标量↔数组映射与二进制编码偏好查询 |
| `lib/pg/reader.cp` | 零拷贝大端消息帧流式读取器 `PgMessageReader`、`PgFrame` 与 `PgBackendTag` |
| `lib/pg/sqlstate.cp` | 5 字符 `SqlStateCode` 严格解析/分类与 15 类 `SqlStateClass` |
| `lib/pg/wiretypes.cp` | 线协议 v3.0 常量、大端整数编解码、`MessageHeader`、`FieldDescription` 与 `StartupMessage` |
| `lib/pg/paramDesc.cp` | 零分配 `ParameterDescription`（`'t'`）参数 OID 数组编解码器与无界零拷贝视图（`ParameterDescriptionView`、`ParameterDescription`） |

## 核心 API 摘要

- **线协议基础 (`wiretypes.cp`)**：`PROTOCOL_VERSION_3_0`, `SSL_REQUEST_CODE`, `CANCEL_REQUEST_CODE`, `PgWireError`, `FormatCode`, `MessageTag`, `WireBuffer`, `readU16BE`/`readI16BE`/`readU32BE`/`readI32BE`, `writeU16BE`/`writeI16BE`/`writeU32BE`/`writeI32BE`, `MessageHeader`, `encodeMessageHeader`/`decodeMessageHeader`, `FieldDescription`, `encodeFieldDescription`/`decodeFieldDescription`, `StartupParam`, `StartupMessage`, `encodeStartupMessage`/`decodeStartupMessage`
- **认证与写入器 (`auth.cp`)**：`AUTH_MESSAGE_TAG`, `PgAuthError`, `AuthType`, `authTypeFromInt`, `SASLContinueInfo`, `DecodedAuthMessage`, `decodeAuthMessage`, `PgWireWriter`
- **COPY / SASL / 协商 (`copysasl.cp`)**：`TAG_COPY_*`, `TAG_PASSWORD_OR_SASL`, `TAG_NEGOTIATE_PROTOCOL_VERSION`, `PgCopySaslError`, `PgFrameBuffer`, `CopyFormat`, `CopyDirection`, `CopyResponse`, `CopyData`, `CopyFail`, `DecodedPasswordMessage`, `DecodedSaslInitialResponse`, `DecodedSaslResponse`, `NegotiateProtocolVersion` 及对应 `encode*`/`decode*` 函数
- **扩展查询 (`extquery.cp`)**：`PgExtendedQueryError`, `TargetKind`, `ParamFormat`, `BindParam`, `ExtendedQueryBuilder`
- **后端消息与诊断 (`messages.cp`)**：`TAG_ERROR_RESPONSE`, `TAG_NOTICE_RESPONSE`, `TAG_BACKEND_KEY_DATA`, `TAG_PARAMETER_STATUS`, `TAG_READY_FOR_QUERY`, `TAG_NOTIFICATION_RESPONSE`, `TAG_COMMAND_COMPLETE`, `PgMessageError`, `FieldType`, `DiagSqlStateClass`, `classifySqlState`, `DiagnosticMessage`, `BackendKeyData`, `ParameterStatus`, `TransactionStatus`, `ReadyForQuery`, `NotificationResponse`, `CommandComplete`, `decodeErrorResponse`/`decodeNoticeResponse`/`decodeBackendKeyData`/`decodeParameterStatus`/`decodeReadyForQuery`/`decodeNotificationResponse`/`decodeCommandComplete`
- **帧读取器 (`reader.cp`)**：`FRAME_HEADER_SIZE`, `PgReaderError`, `PgBackendTag`, `PgFrame`, `PgMessageReader`
- **OID 类型目录 (`oids.cp`)**：`PgOid`, `PgTypeCategory`
- **SQLSTATE 错误码 (`sqlstate.cp`)**：`SqlStateClass`, `SqlStateCode`

## 示例

```cp
import {
    PgOid,
    SqlStateCode,
    SqlStateClass,
    PgWireWriter,
    decodeAuthMessage,
    ExtendedQueryBuilder,
    decodeReadyForQuery,
} from "cap:pg";

fn example(): bool {
    const oid = PgOid.fromU32(23) ?? PgOid.TEXT;
    const is_int = oid == PgOid.INT4;
    if (st := SqlStateCode.parse("23505")) {
        return is_int && st.class() == SqlStateClass.integrity_constraint_violation;
    }
    return false;
}
```

---

## 上游 Bun PostgreSQL 线协议契约与边界注意事项

1. **大端长度前缀包含自身 4 字节（`PgWireWriter` / `PgMessageReader`）**：
   - PostgreSQL Wire Protocol v3 所有常规消息头为 `1B tag + 4B big-endian length`（其中 `length` 包含自身 4 字节，不含 `tag` 字节）；`StartupMessage`、`SSLRequest` 与 `CancelRequest` 无 `1B tag`，首 4 字节即为总长度。
2. **Extended Query 流水线同步边界（`ExtendedQueryBuilder`）**：
   - 批量发送 `Parse -> Bind -> Describe -> Execute` 后必须以 `Sync` 结束事务/查询同步点；服务端返回 `ReadyForQuery`（`'I'` 空闲 / `'T'` 事务中 / `'E'` 失败事务）标志着当前查询管线完整收束。
3. **SQLSTATE 错误分类重试策略（`SqlStateCode`）**：
   - `40001`（serialization_failure）与 `40P01`（deadlock_detected）属可安全重试的并发事务冲突；`23505`（unique_violation）等 `integrity_constraint_violation` 属业务约束冲突不可盲目重试。
4. **COPY 流异常中止（`CopyFail`）**：
   - 在 `CopyInResponse` 写入阶段遇到本地错误时，发送 `encodeCopyFail` 通知服务端安全回滚 COPY 状态，避免连接陷入协议失步。
5. **`ParameterDescription`（`'t'`）预处理参数类型推断与二进制绑定（`paramDesc.cp`，吸收自 `/data/dev/bun/src/sql/postgres/protocol/ParameterDescription.cp` & `/data/dev/bun/docs/runtime/sql.mdx`）**：
   - 在 PostgreSQL Extended Query Protocol 中，客户端发送 `Parse` + `Describe('S', stmt)` 后，服务端返回 `ParameterDescription ('t')` 告知每个 `$1..$N` 占位符的期望 OID；
   - `ParameterDescriptionView` 零拷贝随机访问任意长度参数列表，`allBinaryFriendly()` 与 `hasUnspecifiedTypes()` 帮助驱动决策是否启用零解析开销的二进制参数绑定（`FormatCode.Binary`），同时严格拦截负数计数与截断/越界载荷。

```cp
import { ParameterDescription, ParameterDescriptionView, PgOid } from "cap:pg";

let pd = ParameterDescription.init();
* = pd.push(23)?;   // INT4
* = pd.push(25)?;   // TEXT
* = pd.push(2950)?; // UUID
let wire: [32]u8 = [0; 32];
const n = pd.encodeFrame(wire[0..])?;
const view = ParameterDescriptionView.decodeFrame(wire[0..n])?;
* = view.allBinaryFriendly(); // true
* = PgOid.INT4;
```
