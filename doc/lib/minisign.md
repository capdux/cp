# `cap:minisign` — 零分配 Minisign & OpenBSD Signify 编解码标准库

`cap:minisign` 是纯 CP、零堆分配、零外部依赖（内置 RFC 4648 严格 Base64 编解码）的 Minisign 与 OpenBSD Signify `.pub` / `.minisig` Ed25519 公钥与签名文件编解码库。

---

## 快速导入

```typescript
import {
    MINISIGN_UNTRUSTED_PREFIX,
    MINISIGN_TRUSTED_PREFIX,
    MINISIGN_ALG_LEGACY,
    MINISIGN_ALG_PREHASH,
    MINISIGN_PUBKEY_BIN_LEN,
    MINISIGN_SIG_BIN_LEN,
    MINISIGN_GLOBAL_SIG_BIN_LEN,
    MinisignPublicKey,
    MinisignSignature,
    parseMinisignPublicKeyLine,
    parseMinisignPublicKey,
    parseMinisignSignature,
    formatMinisignPublicKey,
    formatMinisignSignature,
} from "cap:minisign";
```

---

## 常量

- `MINISIGN_UNTRUSTED_PREFIX = "untrusted comment: "`
- `MINISIGN_TRUSTED_PREFIX = "trusted comment: "`
- `MINISIGN_ALG_LEGACY = "Ed"`（纯 Ed25519）
- `MINISIGN_ALG_PREHASH = "ED"`（BLAKE2b-512 预哈希 Ed25519）
- `MINISIGN_PUBKEY_BIN_LEN = 42`（2B `"RW"` + 8B `key_id` + 32B `public_key`）
- `MINISIGN_SIG_BIN_LEN = 74`（2B `"Ed"`/`"ED"` + 8B `key_id` + 64B `signature`）
- `MINISIGN_GLOBAL_SIG_BIN_LEN = 64`

---

## 核心 API

### 公钥：`MinisignPublicKey`
- 字段：`untrusted_comment: []const u8`, `key_id: [8]u8`, `public_key: [32]u8`
- 方法：`keyIdHex(out_16: *[16]u8)`
- 解析：`parseMinisignPublicKey(text)`（2 行 `.pub`）、`parseMinisignPublicKeyLine(b64)`（单行 `-P RW...`）
- 格式化：`formatMinisignPublicKey(comment, key_id, pk, out: *[]u8): usize?`

### 签名：`MinisignSignature`
- 字段：`untrusted_comment: []const u8`, `alg: [2]u8`, `is_prehashed: bool`, `key_id: [8]u8`, `signature: [64]u8`, `trusted_comment: []const u8`, `global_signature: [64]u8`, `has_global_sig: bool`
- 方法：`matchesKeyId(pk: MinisignPublicKey): bool`
- 解析：`parseMinisignSignature(text)`（支持 4 行 Minisign `.minisig` 与 2 行 OpenBSD Signify 签名）
- 格式化：`formatMinisignSignature(untrusted, is_prehashed, key_id, sig, trusted, global_sig, out: *[]u8): usize?`
