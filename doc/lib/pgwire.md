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
