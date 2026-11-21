# `cap:pg_copy_sasl` — 纯 CP PostgreSQL v3 COPY 协议、SASL/Password 前端认证与协议协商编解码库

## 概述

`cap:pg_copy_sasl` 吸收自 Bun `src/sql/postgres/protocol/` 8 个协议模块，补全 PostgreSQL v3 COPY 协议（`CopyInResponse`/`CopyOutResponse`/`CopyData`/`CopyDone`/`CopyFail`）、前端认证报文（`PasswordMessage`/`SASLInitialResponse`/`SASLResponse`）以及 `NegotiateProtocolVersion` 编解码，所有堆分配类型均提供 RAII `~init()` 零泄漏释放。

## 核心类型与 API

- **常量与错误集**：`TAG_COPY_DATA`..`TAG_NEGOTIATE_PROTOCOL_VERSION`、`PgCopySaslError`、`PgFrameBuffer`（`~init()`）
- **COPY 协议**：
  - `CopyFormat`（`text`/`binary`）+ `copyFormatFromByte`、`CopyDirection`（`copy_in`/`copy_out`）
  - `CopyResponse`（`~init()`）+ `encodeCopyResponse` / `decodeCopyInResponse` / `decodeCopyOutResponse`
  - `CopyData` + `encodeCopyData` / `decodeCopyData`
  - `encodeCopyDone` / `decodeCopyDone`
  - `CopyFail` + `encodeCopyFail` / `decodeCopyFail`
- **认证前端报文**：
  - `DecodedPasswordMessage`（`isMd5Hash()`）+ `encodePasswordMessage` / `decodePasswordMessage`
  - `DecodedSaslInitialResponse` + `encodeSaslInitialResponse` / `decodeSaslInitialResponse`
  - `DecodedSaslResponse` + `encodeSaslResponse` / `decodeSaslResponse`
- **协议协商**：
  - `NegotiateProtocolVersion`（`~init()`）+ `encodeNegotiateProtocolVersion` / `decodeNegotiateProtocolVersion`

## 示例

```cp
import { encodeSaslInitialResponse, decodeSaslInitialResponse } from "cap:pg_copy_sasl";

let wire = encodeSaslInitialResponse(alloc, "SCRAM-SHA-256", "n,,n=,r=nonce123")?;
const dec = decodeSaslInitialResponse(wire.slice())?;
// dec.mechanism == "SCRAM-SHA-256"
```
