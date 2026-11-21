# `cap:pg_auth` — 纯 CP PostgreSQL v3 认证握手报文与协议写入器

## 概述

`cap:pg_auth` 吸收自 Bun `src/sql/postgres/protocol/Authentication.cp` 与 `NewWriter.cp`，纯 CP 零外部依赖，提供 PostgreSQL v3 后端认证报文解码器（`Ok` / `CleartextPassword` / `MD5Password` 4 字节 Salt / `SASL` 机制列表 / `SASLContinue` SCRAM 参数 / `SASLFinal` 等）以及支持大端长度头自动回填的 `PgWireWriter`（RAII `~init()` 零泄漏）。

## 核心类型与 API

### 常量与错误集

- `AUTH_MESSAGE_TAG: u8 = 'R'`
- `PgAuthError`：`UnexpectedEof` / `InvalidMessageTag` / `InvalidMessageLength` / `InvalidAuthType` / `InvalidSaslPayload` / `InvalidIterationCount` / `InvalidNullTerminator` / `OutOfBoundsPatch` / `OutOfMemory`

### 认证报文解码

- `AuthType: u32`：`Ok (0)` / `KerberosV5 (2)` / `CleartextPassword (3)` / `MD5Password (5)` / `SCMCredential (6)` / `GSS (7)` / `GSSContinue (8)` / `SSPI (9)` / `SASL (10)` / `SASLContinue (11)` / `SASLFinal (12)`
- `authTypeFromInt(raw: u32): AuthType ! PgAuthError`
- `SASLContinueInfo`：`nonce`、`salt_b64`、`iterations`、`raw`
- `DecodedAuthMessage`（含 `isOk()`、`hasMechanism(name)`、`~init()`）
- `decodeAuthMessage(allocator: std.mem.Allocator, data: []const u8): DecodedAuthMessage ! PgAuthError`

### `PgWireWriter`（大端协议写入器，含 `~init()`）

- `PgWireWriter.init(allocator)` / `bytes()` / `offset()` / `reset()`
- `write` / `writeByte` / `pwrite`
- `writeU16` / `writeI16` / `writeU32` / `writeI32` / `writeU64` / `writeI64` / `writeCString` / `writeNull`
- `beginMessage(tag: u8): usize ! PgAuthError` 与 `endMessage(len_offset: usize): void ! PgAuthError`

## 示例

```cp
import { PgWireWriter, decodeAuthMessage, AUTH_MESSAGE_TAG, AuthType } from "cap:pg_auth";

let w = PgWireWriter.init(alloc);
const pos = w.beginMessage(AUTH_MESSAGE_TAG)?;
w.writeU32(AuthType.MD5Password.toInt())?;
const salt: [4]u8 = [0x11, 0x22, 0x33, 0x44];
w.write(salt[0..])?;
w.endMessage(pos)?;

let msg = decodeAuthMessage(alloc, w.bytes())?;
// msg.auth_type == AuthType.MD5Password && msg.md5_salt[0] == 0x11
```
