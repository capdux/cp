# `cap:otp` — RFC 4226 HOTP, RFC 6238 TOTP 与 `otpauth://` URI 引擎

> **模块路径**: `lib/otp/`  
> **导入协议**: `import { hotp, hotpString, totp, totpString, verifyTotp, base32Encode, base32Decode, formatOtpAuthUri, parseOtpAuthUri, OtpAlgorithm, OtpError, TotpVerifyResult, OtpAuthUri } from "cap:otp";`  
> **动态挂载**: `import cap from "cap";` -> `cap.otp.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:otp` 是纯 CP 实现、零 C 外部依赖（无 `std.posix` / `@import("bun")`）的一次性密码与密钥 URI 标准库，完整实现：

1. **RFC 4648 Base32 编解码 (`base32Encode` / `base32Decode`)**：
   - `base32Encode` 输出标准大写 Base32 并按 8 字符量子进行 `'='` 填充。
   - `base32Decode` 支持大小写不敏感、自动忽略空格 `' '` 与连字符 `'-'`，并严格校验非法字符、错位填充及非法量子长度。
2. **RFC 4226 HOTP (`hotp` / `hotpString`)**：
   - 支持 `Sha1`、`Sha256`、`Sha512`（基于 `std.crypto.auth.hmac`），严格执行动态截断（Dynamic Truncation）与 `6..8` 位前导零补齐。
3. **RFC 6238 TOTP 与恒定时间校验 (`totp` / `totpString` / `verifyTotp`)**：
   - 显式传入 `unix_sec: u64`（不依赖系统时钟），便于确定性单测与无状态服务。
   - `verifyTotp` 使用恒定时间字节比较，支持前后 `±skew_steps` 时间窗口漂移，并返回精确 `matched_step` 与 `delta`，方便上层实现防重放（Replay Protection）。
4. **`otpauth://` Key URI 引擎 (`formatOtpAuthUri` / `parseOtpAuthUri`)**：
   - 完整支持 `otpauth://totp/...` 与 `otpauth://hotp/...` 的百分比编解码、参数校验及 `OtpAuthUri.deinit(allocator)` 确定性内存释放。

---

## 二、 核心 API 速查表 (`lib/otp/otp.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `OtpError` | `InvalidSecret`, `InvalidDigits`, `InvalidPeriod`, `InvalidCode`, `InvalidUri`, `UnsupportedAlgorithm`, `OutOfMemory` |
| `OtpAlgorithm` | 枚举：`Sha1`, `Sha256`, `Sha512` |
| `TotpVerifyResult` | 结构体 `{ valid: bool, matched_step: u64, delta: i64 }` |
| `OtpAuthUri` | 结构体 `{ is_totp: bool, label: []const u8, issuer: []const u8, secret_b32: []const u8, algo: OtpAlgorithm, digits: u8, period: u64, counter: u64 }`；提供 `deinit(*_, allocator)` |
| `base32Encode(allocator, raw)` | 将原始字节编码为 RFC 4648 Base32 字符串（调用方负责 `allocator.free`） |
| `base32Decode(allocator, b32)` | 解码 Base32（大小写不敏感、忽略空格/连字符、严格校验字符与填充） |
| `hotp(secret, counter, digits, algo)` | 计算 RFC 4226 HOTP 整数码（`digits` 为 `6..8`） |
| `hotpString(allocator, secret, counter, digits, algo)` | 计算带前导零补齐的 HOTP 字符串 |
| `totp(secret, unix_sec, period, digits, algo)` | 计算 RFC 6238 TOTP 整数码（`period > 0`） |
| `totpString(allocator, secret, unix_sec, period, digits, algo)` | 计算带前导零补齐的 TOTP 字符串 |
| `verifyTotp(secret, code_str, unix_sec, period, digits, algo, skew_steps)` | 恒定时间校验 TOTP 码，支持 `±skew_steps` 窗口并返回 `TotpVerifyResult` |
| `formatOtpAuthUri(allocator, uri_info)` | 序列化为标准 `otpauth://` URI 字符串 |
| `parseOtpAuthUri(allocator, uri)` | 解析 `otpauth://` URI 为 `OtpAuthUri`（调用方负责 `deinit(allocator)`） |

---

## 三、 使用示例

```typescript
const std = @import("std");
import {
    OtpAlgorithm,
    base32Decode,
    totpString,
    verifyTotp,
    formatOtpAuthUri,
    parseOtpAuthUri,
    OtpAuthUri,
} from "cap:otp";

fn demo(alloc: std.mem.Allocator): !void {
    const secret = base32Decode(alloc, "JBSW Y3DP EHPK 3PXP")?;
    defer alloc.free(secret);

    const code = totpString(alloc, secret, 1700000000, 30, 6, OtpAlgorithm.Sha1)?;
    defer alloc.free(code);

    const res = verifyTotp(secret, code, 1700000030, 30, 6, OtpAlgorithm.Sha1, 1)?;
    // res.valid == true, res.delta == -1

    const uri = formatOtpAuthUri(alloc, OtpAuthUri{
        is_totp: true,
        label: "Acme:alice@example.com",
        issuer: "Acme",
        secret_b32: "JBSWY3DPEHPK3PXP",
        algo: OtpAlgorithm.Sha1,
        digits: 6,
        period: 30,
        counter: 0,
    })?;
    defer alloc.free(uri);

    let parsed = parseOtpAuthUri(alloc, uri)?;
    defer parsed.deinit(alloc);
}
```
