# `cap:s3_signing` — AWS Signature Version 4 (SigV4) S3 请求签名与预签名 URL 引擎

> **模块路径**: `lib/s3_signing/`  
> **导入协议**: `import { S3Credentials, SigV4Header, SigV4QueryParam, SigV4SignedResult, S3SignError, awsUriEncode, deriveSigningKey, signRequest, presignUrl } from "cap:s3_signing";`  
> **动态挂载**: `import cap from "cap";` -> `cap.s3_signing.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:s3_signing` 是纯 CP 实现、零 C 外部依赖（无 `std.posix` / `@import("bun")`）的 AWS Signature Version 4 (SigV4) 请求签名与 S3 预签名 URL 引擎，基于 `std.crypto.hash.sha2.Sha256` 与 `std.crypto.auth.hmac.sha2.HmacSha256` 构建：

1. **RFC 3986 URI 规范编码 (`awsUriEncode`)**：
   - 仅保留非保留字符 `A-Za-z0-9-_.~`，空格编码为 `%20`，所有百分号十六进制均使用大写（`%2F` 等）。
   - 支持 `encode_slash: bool` 控制是否保留路径分隔符 `/`。
2. **4 级链式密钥派生 (`deriveSigningKey`)**：
   - 执行 `HMAC("AWS4" + secret, date) -> kDate -> kRegion -> kService -> kSigning`，输出 32 字节原始派生密钥（支持任意长度 `secret_key` 零堆分配）。
3. **请求签名 (`signRequest`)**：
   - 自动对 Query 参数与 HTTP Headers 按字节序（ASCII 字典序）规范化排序，合并同名多值 Header，自动修剪空白并注入缺失的 `x-amz-date` 与 `x-amz-security-token`。
   - 支持实际载荷与 `"UNSIGNED-PAYLOAD"`，返回包含 `authorization`、`amz_date`、`payload_sha256`、`canonical_request`、`string_to_sign`、`signature` 的 `SigV4SignedResult`（需调用 `deinit(allocator)` 释放堆缓冲区）。
4. **预签名 URL (`presignUrl`)**：
   - 严格校验 `1 <= expires_sec <= 604800`（7 天上限），使用 `UNSIGNED-PAYLOAD` 生成包含 `X-Amz-Algorithm`、`X-Amz-Credential`、`X-Amz-Date`、`X-Amz-Expires`、`X-Amz-SignedHeaders`、可选 `X-Amz-Security-Token`、自定义扩展参数及 `X-Amz-Signature` 的完整预签名 URL。

---

## 二、 核心 API 速查表 (`lib/s3_signing/s3_signing.cp`)

| 符号 / 函数 | 签名与说明 |
| :--- | :--- |
| `S3SignError` | `InvalidCredentials`, `InvalidTimestamp`, `InvalidRegion`, `InvalidService`, `InvalidMethod`, `InvalidPath`, `InvalidExpiration`, `OutOfMemory` |
| `S3Credentials` | `{ access_key_id: []const u8, secret_access_key: []const u8, session_token: ([]const u8)? = null }` |
| `SigV4Header` | `{ name: []const u8, value: []const u8 }` |
| `SigV4QueryParam` | `{ name: []const u8, value: []const u8 }` |
| `SigV4SignedResult` | `{ authorization: []u8, amz_date: [16]u8, payload_sha256: [64]u8, canonical_request: []u8, string_to_sign: []u8, signature: [64]u8 }`；提供 `deinit(*_, allocator)` |
| `awsUriEncode(allocator, input, encode_slash)` | 返回 `S3SignError![]u8`，严格执行 AWS SigV4 RFC 3986 编码 |
| `deriveSigningKey(secret_key, date_yyyymmdd, region, service)` | 返回 `[32]u8`，4 级链式 HMAC-SHA256 派生签名密钥 |
| `signRequest(allocator, method, canonical_uri, query_params, headers, payload_or_unsigned, creds, region, service, amz_date_iso8601)` | 返回 `S3SignError!SigV4SignedResult` |
| `presignUrl(allocator, scheme, host, canonical_uri, method, expires_sec, extra_query_params, creds, region, service, amz_date_iso8601)` | 返回 `S3SignError![]u8`，生成完整 S3 预签名 URL |

---

## 三、 使用示例

```typescript
const std = @import("std");
import { S3Credentials, SigV4Header, SigV4QueryParam, signRequest, presignUrl } from "cap:s3_signing";

fn demo(alloc: std.mem.Allocator): !void {
    const creds = S3Credentials{
        .access_key_id = "AKIDEXAMPLE",
        .secret_access_key = "wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY",
    };

    const headers = [
        SigV4Header{ .name = "Host", .value = "my-bucket.s3.us-east-1.amazonaws.com" },
    ];
    const query = [SigV4QueryParam]{};

    let signed = signRequest(
        alloc,
        "GET",
        "/hello.txt",
        &query,
        &headers,
        "UNSIGNED-PAYLOAD",
        creds,
        "us-east-1",
        "s3",
        "20250101T000000Z",
    )?;
    defer signed.deinit(alloc);

    const url = presignUrl(
        alloc,
        "https",
        "my-bucket.s3.us-east-1.amazonaws.com",
        "/hello.txt",
        "GET",
        3600,
        &query,
        creds,
        "us-east-1",
        "s3",
        "20250101T000000Z",
    )?;
    defer alloc.free(url);
}
```
