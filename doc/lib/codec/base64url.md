# `cap:base64url` — 纯 CP RFC 4648 Base64 & Base64URL 无填充/自动对齐编解码引擎

> [!NOTE]
> 本子模块已归并至统一主包 `cap:codec`，导入请使用 `import { ... } from "cap:codec"`。

## 概述

`cap:base64url`（亦支持 `cap.base64url` 动态挂载）基于 Zig 0.17 `std.base64` 实现 RFC 4648 Standard Base64、JWT/JWK 专用无填充 Base64URL (`encodeUrlNoPad`/`decodeUrlNoPad`)、带填充 Base64URL (`encodeUrlPadded`) 与自适应对齐解码 (`decodeUrlAuto`)。

## 核心类型与 API

- `Base64UrlError`: `InvalidCharacter`、`InvalidPadding`、`InvalidLength`、`BufferTooSmall`
- `encodeStandard(alloc, data)` / `decodeStandard(alloc, encoded)`
- `encodeUrlNoPad(alloc, data)` / `decodeUrlNoPad(alloc, encoded)`
- `encodeUrlPadded(alloc, data)` / `decodeUrlAuto(alloc, encoded)`

## 示例

```cp
import { encodeUrlNoPad, decodeUrlAuto } from "cap:codec";

const tok = try encodeUrlNoPad(alloc, "{\"alg\":\"EdDSA\"}");
defer alloc.free(tok);
const raw = try decodeUrlAuto(alloc, tok);
defer alloc.free(raw);
```
