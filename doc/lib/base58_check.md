# `cap:base58_check` — 纯 CP Bitcoin Base58 & Base58Check 双重 SHA-256 编解码引擎

## 概述

`cap:base58_check`（亦支持 `cap.base58_check` 动态挂载）实现完整 Base58 大整数进位编解码与 Bitcoin Base58Check（版本号 + 载荷 + 4 字节 Double-SHA256 校验和）编解码与篡改拦截。

## 核心类型与 API

- `Base58Error`: `InvalidCharacter`、`InvalidChecksum`、`PayloadTooShort`
- `CheckDecoded`: `{ version: u8, payload: []u8 }`（含 `deinit(alloc)`）
- `encode(alloc, data) -> ![]u8` / `decode(alloc, encoded) -> ![]u8`
- `encodeCheck(alloc, version, payload) -> ![]u8` / `decodeCheck(alloc, encoded) -> !CheckDecoded`

## 示例

```cp
import { encodeCheck, decodeCheck } from "cap:base58_check";

const addr = try encodeCheck(alloc, 0x00, &pubkey_hash);
defer alloc.free(addr);
const dec_res = try decodeCheck(alloc, addr);
defer dec_res.deinit(alloc);
```
