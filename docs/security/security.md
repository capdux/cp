# CP 标准库：Web 安全防护与 CSRF 令牌子系统手册 (`cap:security`)

## 一、 架构总览与设计准则

`cap:security` 是 CP 语言官方内置的 Web 安全与密码学令牌认证标准库，全面吸收自 Bun 原生 CSRF 防护模块（`/data/dev/bun/src/csrf/csrf.cp`）。

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        CP 用户层应用 (Application)                      │
├───────────────────────────────────┬────────────────────────────────────┤
│     编码令牌接口 (CSRF.generate)   │     编码校验接口 (CSRF.verify)     │
│  Base64Url / Base64 / Hex 格式    │  自动解码 + 签名比对 + 有效期验证  │
├───────────────────────────────────┴────────────────────────────────────┤
│         64 字节二进制紧凑令牌引擎 (generateRaw / verifyRaw)            │
│   [8B Timestamp BE] + [16B CSPRNG Nonce] + [8B ExpiresIn BE] + [32B]   │
├────────────────────────────────────────────────────────────────────────┤
│     底层密码学原语: cap:crypto (hmacSha256Raw / fillRandom)            │
│     时序攻击防护: constantTimeEql (恒定时间按位异或比较)               │
└────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

`lib/security/` 下所有源文件第 1 行均严格标记上游 Bun 源码来源：

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/security/csrf.cp` | `// Source: /data/dev/bun/src/csrf/csrf.cp` | `CSRF` 令牌签发与验证、`TokenFormat` 编码枚举、`DEFAULT_EXPIRATION_MS` 与 `constantTimeEql` 恒定时间比对 |
| `lib/security/index.cp` | `// Source: /data/dev/bun/src/csrf/csrf.cp` | `cap:security` 标准库对外统一导出入口 |

---

## 二、 CSRF Token 64 字节二进制载荷结构

每个由 `cap:security` 签发的 CSRF Token 在编码前均由固定 **64 字节（512 bits）** 二进制结构组成，无需服务端 Session 存储即可实现无状态（Stateless）防伪与过期校验：

```text
 0                   8                        24                  32                                  64
 ┌───────────────────┬────────────────────────┬───────────────────┬───────────────────────────────────┐
 │   Timestamp (ms)  │     CSPRNG Nonce       │  ExpiresIn (ms)   │       HMAC-SHA256 Signature       │
 │  u64 Big-Endian   │   16 Bytes Random      │  u64 Big-Endian   │     32 Bytes over Bytes [0..32]   │
 └───────────────────┴────────────────────────┴───────────────────┴───────────────────────────────────┘
 ◄────────────────────────── 32-Byte Signed Payload ──────────────►◄───────── 32-Byte MAC Tag ────────►
```

1. **`Timestamp` (`[0..8]`, 8 字节大端序 `u64`)**：令牌签发时的 Unix 毫秒时间戳（`CLOCK_REALTIME`）；
2. **`CSPRNG Nonce` (`[8..24]`, 16 字节)**：由密码学安全伪随机数发生器（`fillRandom`）填充的 128-bit 随机盐值，确保同一毫秒内签发的 Token 绝不重复，防止 BREACH 压缩侧信道分析；
3. **`ExpiresIn` (`[24..32]`, 8 字节大端序 `u64`)**：令牌内嵌的有效期毫秒数（默认 `DEFAULT_EXPIRATION_MS = 86_400_000`，即 24 小时），受 HMAC 签名保护不可篡改；
4. **`HMAC-SHA256 Signature` (`[32..64]`, 32 字节)**：使用服务端 `secret` 对前 32 字节载荷计算的 `HMAC-SHA256` 认证标签。

---

## 三、 HMAC 签名验证、过期防回绕与防时序侧信道攻击

`CSRF.verify` 与 `CSRF.verifyRaw` 在校验令牌时执行严格的多层安全防线：

1. **恒定时间 MAC 比对 (`constantTimeEql`)**：
   - 重新计算前 32 字节的期望 `HMAC-SHA256`，并通过 `diff |= (a[i] ^ b[i])` 遍历全部 32 字节；
   - 执行耗时与失配字节位置完全无关，彻底消除通过响应延迟猜解签名秘钥的时序侧信道隐患（Timing Side-Channel）。
2. **时钟偏移防护（Clock Skew Guard）**：
   - 若令牌中的 `timestamp > now + 60000`（超前当前时间 60 秒以上），立即判定为非法令牌拒绝。
3. **无符号整数溢出防回绕与双重过期校验**：
   - 校验 `std.math.maxInt(u64) - timestamp < expires_in`，防止超大 `expires_in` 导致 `timestamp + expires_in` 整数回绕；
   - 同时检查令牌内嵌 `expires_in` 与验证方指定的 `max_age_ms`，任一超时即拒绝。

---

## 四、 完整代码示例

```typescript
import {
    CSRF,
    TokenFormat,
    DEFAULT_EXPIRATION_MS,
    constantTimeEql,
} from "cap:security";
const std = @import("std");

pub fn demoCsrfSecurity(): !void {
    const alloc = std.heap.page_allocator;
    const secret = "production_signing_secret_key_2026_x99";

    // =========================================================================
    // 1. 生成 URL-Safe Base64 格式的 CSRF Token（适用于 HTTP Header / Form 隐藏域）
    // =========================================================================
    const token_b64url = CSRF.generate(
        secret,
        DEFAULT_EXPIRATION_MS,
        TokenFormat.Base64Url,
        alloc
    )?;
    defer alloc.free(token_b64url);

    // 验证合法 Token
    const is_ok = CSRF.verify(
        token_b64url,
        secret,
        DEFAULT_EXPIRATION_MS,
        TokenFormat.Base64Url,
        alloc
    );
    // is_ok => true

    // =========================================================================
    // 2. 生成 Hex 格式的 CSRF Token (固定 128 个十六进制字符)
    // =========================================================================
    const token_hex = CSRF.generate(secret, 60000, TokenFormat.Hex, alloc)?;
    defer alloc.free(token_hex);
    // token_hex.len == 128

    // =========================================================================
    // 3. 零堆分配原始 64 字节二进制 Token 生成与确定性时间戳校验
    // =========================================================================
    let raw_token: [64]u8 = undefined;
    const issued_at: u64 = 1700000000000;
    const ttl_ms: u64 = 300000; // 5 分钟有效期

    * = CSRF.generateRawAt(secret, issued_at, ttl_ms, &raw_token)?;

    // 有效期内 (issued_at + 120s) -> true
    const valid_now = CSRF.verifyRawAt(&raw_token, secret, issued_at + 120000, ttl_ms);

    // 过期后 (issued_at + 301s) -> false
    const expired_later = CSRF.verifyRawAt(&raw_token, secret, issued_at + 301000, ttl_ms);

    // =========================================================================
    // 4. 恒定时间安全字符串/摘要比对
    // =========================================================================
    const match = constantTimeEql("webhook_sig_a1b2", "webhook_sig_a1b2"); // true
}
```

---

## 五、 与 `Bun.CSRF` API 对照表

| 功能特性 | Bun (`Bun.CSRF` / `src/csrf/csrf.cp`) | CP 标准库 (`cap:security`) |
| :--- | :--- | :--- |
| **生成编码 Token** | `Bun.CSRF.generate(secret, { encoding, expiresIn })` | `CSRF.generate(secret, expires_in_ms, format, alloc)` |
| **验证编码 Token** | `Bun.CSRF.verify(token, { secret, encoding, maxAge })` | `CSRF.verify(token, secret, max_age_ms, format, alloc)` |
| **零分配二进制生成** | `CSRF.generate(options, &out_buffer)` | `CSRF.generateRaw(secret, expires_in_ms, &out)` / `CSRF.generateRawAt(...)` |
| **零分配二进制校验** | `CSRF.verify(options)` | `CSRF.verifyRaw(raw, secret, max_age_ms)` / `CSRF.verifyRawAt(...)` |
| **支持的编码格式** | `"base64url"` / `"base64"` / `"hex"` | `TokenFormat.Base64Url` / `TokenFormat.Base64` / `TokenFormat.Hex` |
| **默认过期时间** | `DEFAULT_EXPIRATION_MS` (`86400000` ms) | `DEFAULT_EXPIRATION_MS` (`86400000` ms) |
| **恒定时间比对** | `boring.CRYPTO_memcmp` | `constantTimeEql(a, b)` |
