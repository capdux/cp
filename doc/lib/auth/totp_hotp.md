# cap:totp_hotp — RFC 4226 (HOTP) & RFC 6238 (TOTP) 一次性动态口令标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:auth`，导入请使用 `import { ... } from "cap:auth"`。

## 1. 概述

`cap:totp_hotp`（同时支持 `import cap from "cap"; cap.totp_hotp` 动态挂载）提供纯 CP 实现的工业级动态口令（OTP）引擎，完整遵循 **RFC 4226 (HOTP)**、**RFC 6238 (TOTP)**、**RFC 4648 (Base32)** 与 **`otpauth://` URI** 规范，支持 SHA-1 / SHA-256 / SHA-512 HMAC、重同步窗口（resync lookahead）、时间漂移容忍（drift window）以及常量时间口令比对。

## 2. 核心类型

- `HashAlg`: `Sha1 | Sha256 | Sha512` — HMAC 哈希算法枚举。
- `OtpType`: `Totp | Hotp` — `otpauth://` 口令类型枚举。
- `OtpAuthUri`:
  - `otp_type: OtpType`
  - `label: []const u8`
  - `issuer: []const u8`
  - `account: []const u8`
  - `secret: []const u8`（Base32 编码密钥）
  - `algorithm: HashAlg`
  - `digits: u8`（6..8）
  - `period: u64`（默认 30 秒）
  - `counter: u64`（HOTP 初始计数器）
  - `deinit(self: *OtpAuthUri, alloc: std.mem.Allocator)`：释放解析分配的所有字符串内存。

## 3. API 速查

- `hotpAt(secret: []const u8, counter: u64, digits: u8) u32` — 计算 SHA-1 HOTP 数值。
- `hotpAtWithAlg(secret: []const u8, counter: u64, digits: u8, alg: HashAlg) u32` — 指定哈希算法计算 HOTP 数值。
- `totpAt(secret: []const u8, unix_sec: u64, period: u64, digits: u8, alg: HashAlg) u32` — 计算指定 Unix 时间戳的 TOTP 数值。
- `formatCode(code: u32, digits: u8, out: []u8) []const u8` — 将口令数值零填充格式化为固定长度字符串。
- `constantTimeEq(a: []const u8, b: []const u8) bool` — 常量时间字节切片比对。
- `verifyHotp(secret, counter, code, digits, lookahead, alg) u64?` — 在 `[counter, counter + lookahead]` 窗口内常量时间校验 HOTP，匹配返回新 `counter + 1`，否则返回 `null`。
- `verifyTotp(secret, unix_sec, code, period, digits, window, alg) i64?` — 在 `[-window, +window]` 步数窗口内常量时间校验 TOTP，匹配返回步数漂移 `delta`，否则返回 `null`。
- `base32Encode(alloc, data, pad) []u8?` / `base32Decode(alloc, input) []u8?` — RFC 4648 Base32 编解码（解码忽略大小写、空白与 `=` 填充）。
- `generateSecret(alloc, num_bytes) []u8?` — 基于 `std.Io.Threaded` 生成加密安全随机密钥字节。
- `buildOtpAuthUri(alloc, uri) []u8?` / `parseOtpAuthUri(alloc, uri_str) OtpAuthUri?` — `otpauth://` URI 构造与解析。

## 4. 示例

```typescript
import { totpAt, verifyTotp, formatCode, HashAlg } from "cap:auth";

const secret = "12345678901234567890";
const code = totpAt(secret, 59, 30, 8, HashAlg.Sha1); // 94287082
let buf: [8]u8 = undefined;
const code_str = formatCode(code, 8, &buf);
const delta = verifyTotp(secret, 89, code_str, 30, 8, 1, HashAlg.Sha1); // -1
```
