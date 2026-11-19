# CP 标准库：密码学哈希、AEAD 加密、密钥派生与 TLS 身份校验手册 (`cap:crypto`)

## 一、 架构总览与设计准则

`cap:crypto` 是 CP 语言官方内置的工业级现代密码学与 TLS 安全子系统标准库，全面吸收自 Bun 原生密码学与 BoringSSL 绑定层（`/data/dev/bun/src/boringssl/boringssl.cp` 与 `src/sha_hmac/`）。

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        CP 用户层应用 (Application)                      │
├───────────────────────────────────┬────────────────────────────────────┤
│     消息摘要与认证 (Hash / HMAC)   │   AEAD 对称加密 (Cipher Engine)    │
│  SHA-256 / SHA-512 / SHA-1 / MD5  │  Aes256Gcm / ChaCha20Poly1305      │
│  createHash / createHmac (流式)   │  12B Nonce + 16B Auth Tag 防篡改   │
├───────────────────────────────────┼────────────────────────────────────┤
│     密钥派生与随机数 (KDF / RNG)   │   TLS & X.509 身份校验 (BoringSSL) │
│  pbkdf2Sync / pbkdf2Into / Raw    │  TlsHostnameVerifier (RFC 6125)    │
│  randomBytes / fillRandom / UUIDv4│  matchDnsName / checkServerIdentity│
└───────────────────────────────────┴────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

`lib/crypto/` 下所有 `.cp` 源文件第 1 行均严格标记上游 Bun 源码来源：

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/crypto/hash.cp` | `// Source: /data/dev/bun/src/boringssl/boringssl.cp` | 单发与流式哈希（`sha256`, `sha512`, `sha1`, `md5`, `sha256Raw`, `Hash`, `createHash`） |
| `lib/crypto/hmac.cp` | `// Source: /data/dev/bun/src/boringssl/boringssl.cp` | 单发与流式消息认证码（`hmacSha256`, `hmacSha256Raw`, `hmacSha512`, `Hmac`, `createHmac`） |
| `lib/crypto/pbkdf2.cp` | `// Source: /data/dev/bun/src/boringssl/boringssl.cp` | 基于密码的密钥派生函数 2（`pbkdf2Sync`, `pbkdf2Raw`, `pbkdf2Into` 零分配栈版） |
| `lib/crypto/cipher.cp` | `// Source: /data/dev/bun/src/boringssl/boringssl.cp` | AEAD 认证对称加密引擎（`Aes256Gcm`, `ChaCha20Poly1305`） |
| `lib/crypto/random.cp` | `// Source: /data/dev/bun/src/boringssl/boringssl.cp` | 内核级 CSPRNG（`getrandom` 驱动的 `fillRandom`, `randomBytes`, `randomUUID`, `randomUUIDInto`, `randomInt`） |
| `lib/crypto/tls.cp` | `// Source: /data/dev/bun/src/boringssl/boringssl.cp` | RFC 6125 TLS 主机名通配符验证器 `TlsHostnameVerifier` 与 BoringSSL X.509 证书链校验 |
| `lib/crypto/_boringssl.cp` | `// Source: /data/dev/bun/src/boringssl/boringssl.cp` | 底层 BoringSSL C 头文件桥接声明 |
| `lib/crypto/boringssl.cp` | `// Source: /data/dev/bun/src/boringssl/boringssl.cp` | BoringSSL 导出包装层 |
| `lib/crypto/index.cp` | `// Source: /data/dev/bun/src/boringssl/boringssl.cp` | `cap:crypto` 标准库对外统一导出入口 |

---

## 二、 核心密码学原语详解

### 1. 密码学哈希与流式摘要 (`Hash` & `createHash`)

- **单发极速 API**：`sha256(data)`、`sha512(data)`、`sha1(data)`、`md5(data)` 直接返回小写十六进制摘要字符串；
- **零堆分配原始字节版**：`sha256Raw(&out32, data)`、`sha512Raw(&out64, data)`、`sha1Raw(&out20, data)`、`md5Raw(&out16, data)` 直接将二进制摘要写入调用方栈数组；
- **流式分块摘要 (`createHash`)**：支持任意次数 `.update(chunk)` 链式调用，最终调用 `.digest()` 产出十六进制摘要。

### 2. 消息认证码 (`Hmac` & `createHmac`) 与密钥派生 (`PBKDF2`)

- **HMAC-SHA256 / HMAC-SHA512**：提供 `hmacSha256(key, data)`、`hmacSha256Raw(&out32, key, data)`、`hmacSha512(key, data)` 及流式 `createHmac("sha256", key)`；
- **PBKDF2-HMAC-SHA256 (RFC 8018)**：
  - `pbkdf2Sync(password, salt, iterations, keylen)`：派生指定字节长度密钥并编码为 Hex；
  - `pbkdf2Raw(password, salt, iterations, keylen)`：返回原始派生密钥字节切片；
  - `pbkdf2Into(out_slice, password, salt, iterations)`：零堆分配直接派生至栈缓冲区。

### 3. AEAD 认证对称加密 (`Aes256Gcm` & `ChaCha20Poly1305`)

两种工业级 AEAD 算法均采用 **32 字节密钥（256-bit Key）**、**12 字节随机数（96-bit Nonce）** 与 **16 字节认证标签（128-bit Poly1305/GHASH Tag）**：
- 解密时若密文（`ciphertext`）、附加认证数据（`ad`）或标签（`tag`）发生任何 1 比特篡改，`decrypt()` 均在恒定时间内返回 `false`，绝不泄露未认证明文。

### 4. RFC 6125 TLS 主机名与通配符校验 (`TlsHostnameVerifier`)

严格遵循 **RFC 6125 Section 6.4.3** 证书 Subject Alternative Name (SAN) / Common Name (CN) 匹配准则：
- `isSafeAltName(name, allow_ip)`：拦截含空字节 `\0`、空白符、超长（`> 253`）或伪造冒号的畸形域名；
- `matchDnsName(pattern, hostname)` / `TlsHostnameVerifier.verify(pattern, hostname)`：
  - 仅允许最左侧单标签通配符（如 `*.example.com` 匹配 `api.example.com`）；
  - **严格拒绝跨级子域匹配**（`*.example.com` 不匹配 `sub.api.example.com`）；
  - **严格拒绝顶级域宽泛通配**（如 `*.com` 不匹配任何主机名）。

---

## 三、 完整代码示例

```typescript
import {
    sha256,
    sha256Raw,
    createHash,
    hmacSha256,
    createHmac,
    pbkdf2Sync,
    pbkdf2Into,
    Aes256Gcm,
    ChaCha20Poly1305,
    randomBytes,
    randomUUID,
    randomUUIDInto,
    TlsHostnameVerifier,
} from "cap:crypto";

pub fn demoCryptoFull(): !void {
    // =========================================================================
    // 1. 哈希与流式 HMAC
    // =========================================================================
    const digest = sha256("hello world");
    // => "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9"

    let h = createHash("sha256").?;
    h.update("hello ").update("world");
    // h.digest() == digest

    let mac = createHmac("sha256", "jwt_secret").?;
    mac.update("header.payload");
    const sig = mac.digest();

    // =========================================================================
    // 2. PBKDF2 零堆分配密钥派生 + AES-256-GCM 认证加密
    // =========================================================================
    let derived_key: [32]u8 = undefined;
    * = pbkdf2Into(&derived_key, "master_password", "unique_salt_16b", 10000);

    let nonce: [12]u8 = [7; 12];
    const plaintext = "Confidential AI Agent State";
    const aad = "tenant:deepmind-01";
    let ciphertext: [27]u8 = undefined;
    let tag: [16]u8 = undefined;

    Aes256Gcm.encrypt(&derived_key, &nonce, plaintext, aad, &ciphertext, &tag);

    let recovered: [27]u8 = undefined;
    const verified = Aes256Gcm.decrypt(&derived_key, &nonce, &ciphertext, tag, aad, &recovered);
    // verified == true, recovered == plaintext

    // =========================================================================
    // 3. CSPRNG UUIDv4 与 RFC 6125 TLS 域名通配符校验
    // =========================================================================
    let uuid_stack: [36]u8 = undefined;
    randomUUIDInto(&uuid_stack);
    // uuid_stack[14] == '4' (RFC 4122 Version 4)

    const ok_sub = TlsHostnameVerifier.verify("*.cp-lang.org", "pkg.cp-lang.org"); // true
    const bad_deep = TlsHostnameVerifier.verify("*.cp-lang.org", "a.b.cp-lang.org"); // false
}
```

---

## 四、 与 `Bun.CryptoHasher` / `node:crypto` API 对照表

| 功能特性 | Node.js (`node:crypto`) | Bun (`Bun.CryptoHasher` / `src/boringssl`) | CP 标准库 (`cap:crypto`) |
| :--- | :--- | :--- | :--- |
| **单次 SHA-256 哈希** | `crypto.createHash("sha256").update(d).digest("hex")` | `new Bun.CryptoHasher("sha256").update(d).digest("hex")` | `sha256(d)` / `sha256Raw(&out, d)` |
| **流式哈希计算** | `crypto.createHash(algo)` | `new Bun.CryptoHasher(algo)` | `createHash(algo)` -> `.update(d)` -> `.digest()` |
| **HMAC 消息认证码** | `crypto.createHmac("sha256", key)` | `new Bun.CryptoHasher("sha256", key)` | `hmacSha256(key, d)` / `createHmac(algo, key)` |
| **PBKDF2 密钥派生** | `crypto.pbkdf2Sync(pw, salt, iter, len, "sha256")` | `crypto.pbkdf2Sync(...)` | `pbkdf2Sync(pw, salt, iter, len)` / `pbkdf2Into(&out, ...)` |
| **AES-256-GCM 加密** | `crypto.createCipheriv("aes-256-gcm", k, iv)` | BoringSSL `EVP_AEAD_CTX` | `Aes256Gcm.encrypt` / `Aes256Gcm.decrypt` |
| **ChaCha20-Poly1305** | `crypto.createCipheriv("chacha20-poly1305", ...)` | BoringSSL `EVP_aead_chacha20_poly1305` | `ChaCha20Poly1305.encrypt` / `ChaCha20Poly1305.decrypt` |
| **CSPRNG 随机字节** | `crypto.randomBytes(n)` / `crypto.randomFillSync(buf)` | `crypto.getRandomValues(buf)` | `randomBytes(n)` / `fillRandom(buf)` |
| **RFC 4122 UUIDv4** | `crypto.randomUUID()` | `crypto.randomUUID()` | `randomUUID()` / `randomUUIDInto(&buf36)` |
| **TLS 证书主机名校验** | `tls.checkServerIdentity(host, cert)` | `BoringSSL.checkServerIdentity` / `matchDnsName` | `TlsHostnameVerifier.verify(pattern, host)` / `matchDnsName` |
