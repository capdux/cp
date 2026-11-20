# `cap:sha_hmac` — 统一密码学哈希、HMAC、HKDF 与 PBKDF2 标准库

## 1. 概述

`cap:sha_hmac`（支持 `import { ... } from "cap:sha_hmac"` 与 `import cap from "cap"; cap.sha_hmac.*`）提供纯 CP 工业级密码学摘要、消息认证码与密钥派生原语，底层复用 Zig `std.crypto` 经验证的原语，无外部 C/BoringSSL 依赖。

- **哈希算法 (`HashAlg`)**：`Sha1`, `Sha224`, `Sha256`, `Sha384`, `Sha512`, `Blake2b256`, `Blake2s256`
- **流式与单次摘要**：`StreamingHasher`, `hashOneshot`, `hashHex`, `digestHex`
- **HMAC 与常量时间验签**：`StreamingHmac`, `hmacOneshot`, `hmacHex`, `verifyHmac`（基于 `std.crypto.timing_safe.eql`）
- **密钥派生 (KDF)**：RFC 5869 `hkdfExtract` / `hkdfExpand` / `hkdf`（强制校验 `out_len <= 255 * HashLen`），以及 RFC 2898 `pbkdf2HmacSha256` / `pbkdf2Hmac`（强制校验 `iterations >= 1`）

## 2. 核心类型

- `ShaError`: `error{ InvalidLength, InvalidIterations, OutputTooLong, BufferTooSmall, OutOfMemory }`
- `HashAlg`: `enum { Sha1, Sha224, Sha256, Sha384, Sha512, Blake2b256, Blake2s256 }`
- `StreamingHasher`: 状态机封装的增量哈希器（`init` -> `update` -> `final` / `finalHex`）
- `StreamingHmac`: 状态机封装的增量 HMAC 计算器（`init` -> `update` -> `final` / `finalHex`）

## 3. API 速查

### 哈希摘要 (`hash.cp`)
| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `digestLen` | `(alg: HashAlg): usize` | 返回摘要字节长度（20/28/32/48/64） |
| `blockLen` | `(alg: HashAlg): usize` | 返回内部块字节长度（64/128） |
| `StreamingHasher.init` | `(alg: HashAlg): StreamingHasher` | 初始化流式哈希器 |
| `StreamingHasher.update` | `(*_, data: []const u8)` | 增量输入数据分片 |
| `StreamingHasher.final` | `(*_, out_buf: []u8): ShaError![]const u8` | 输出原始摘要切片 |
| `StreamingHasher.finalHex` | `(*_, alloc: std.mem.Allocator): ![]u8` | 输出小写 Hex 摘要字符串 |
| `hashOneshot` | `(alloc, alg: HashAlg, data: []const u8): ![]u8` | 单次计算原始摘要 |
| `hashHex` | `(alloc, alg: HashAlg, data: []const u8): ![]u8` | 单次计算小写 Hex 摘要 |
| `digestHex` | `(alloc, digest: []const u8): ![]u8` | 原始字节转小写 Hex |

### HMAC 与验签 (`hmac.cp`)
| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `StreamingHmac.init` | `(alg: HashAlg, key: []const u8): StreamingHmac` | 使用密钥初始化流式 HMAC |
| `StreamingHmac.update` | `(*_, data: []const u8)` | 增量输入消息分片 |
| `StreamingHmac.final` | `(*_, out_buf: []u8): ShaError![]const u8` | 输出认证标签 |
| `StreamingHmac.finalHex` | `(*_, alloc: std.mem.Allocator): ![]u8` | 输出小写 Hex 标签 |
| `hmacOneshot` | `(alloc, alg, key, msg): ![]u8` | 单次计算 HMAC 标签 |
| `hmacHex` | `(alloc, alg, key, msg): ![]u8` | 单次计算小写 Hex HMAC |
| `verifyHmac` | `(alg, key, msg, expected_tag): bool` | 常量时间 HMAC 验签 |

### 密钥派生 (`kdf.cp`)
| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `hkdfExtract` | `(alloc, alg, salt, ikm): ![]u8` | RFC 5869 Extract 阶段，返回 PRK |
| `hkdfExpand` | `(alloc, alg, prk, info, out_len): ![]u8` | RFC 5869 Expand 阶段（校验 `out_len <= 255 * HashLen`） |
| `hkdf` | `(alloc, alg, salt, ikm, info, out_len): ![]u8` | 完整 Extract-then-Expand |
| `pbkdf2HmacSha256` | `(alloc, password, salt, iterations, out_len): ![]u8` | RFC 2898 PBKDF2-HMAC-SHA256 |
| `pbkdf2Hmac` | `(alloc, alg, password, salt, iterations, out_len): ![]u8` | 通用 PBKDF2-HMAC（校验 `iterations >= 1`） |

## 4. 示例

```cp
import { HashAlg, hashHex, hmacHex, verifyHmac, hkdf, pbkdf2HmacSha256 } from "cap:sha_hmac";
const std = @import("std");

pub fn example(alloc: std.mem.Allocator): !void {
    const h = try hashHex(alloc, .Sha256, "hello");
    defer alloc.free(h);

    const tag = try hmacHex(alloc, .Sha256, "secret", "payload");
    defer alloc.free(tag);

    const okm = try hkdf(alloc, .Sha256, "salt", "ikm", "info", 32);
    defer alloc.free(okm);

    const dk = try pbkdf2HmacSha256(alloc, "password", "salt", 4096, 32);
    defer alloc.free(dk);
}
```
