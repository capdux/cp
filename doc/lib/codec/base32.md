# `cap:base32` — 纯 CP RFC 4648 Base32 / Base32hex / Crockford Base32 编解码引擎

> [!NOTE]
> 本子模块已归并至统一主包 `cap:codec`，导入请使用 `import { ... } from "cap:codec"`。

## 概述

`cap:base32`（亦支持 `cap.base32` 动态挂载）实现 RFC 4648 Standard Base32、Base32hex 与 Crockford Base32 三种字母表编解码，支持可选 `=` 填充、大小写不敏感解码及 Crockford `O->0`/`I,L->1` 容错与 `-` 分隔符忽略。

## 核心类型与 API

- `Variant`: `.Standard` | `.Hex` | `.Crockford`
- `Base32Error`: `InvalidCharacter`、`InvalidPadding`、`InvalidLength`、`BufferTooSmall`
- `encodedLen(input_len, padded) -> usize`
- `encodeAlloc(alloc, data, variant, padded) -> ![]u8`
- `decodeAlloc(alloc, encoded, variant) -> ![]u8`

## 示例

```cp
import { encodeAlloc, decodeAlloc } from "cap:codec";

const enc = try encodeAlloc(alloc, "foobar", .Standard, true);
defer alloc.free(enc);
const orig = try decodeAlloc(alloc, enc, .Standard);
defer alloc.free(orig);
```
