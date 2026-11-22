# CP 标准库：全栈现代密码学、非对称签名、KDF 与 JWT/PEM 手册 (`cap:crypto`)

> 参考来源：`/data/dev/bun/docs/runtime/hashing.mdx`、`/data/dev/bun/src/boringssl/boringssl.cp`、`/data/dev/bun/src/sha_hmac/hmac.cp` | 吸收日期：2026-11-22

## 一、 架构总览与设计准则

`cap:crypto` 是 CP 语言官方内置的生产级零 C 依赖全栈现代密码学子系统标准库，结合 Zig 0.17 原生 `std.crypto` 与 Bun `src/boringssl/`、`src/sha_hmac/` 现代密码学 API 契约，覆盖**消息摘要、HMAC、KDF/口令哈希、AEAD 对称加密、非对称签名/密钥协商、JWT/PEM 编解码、CSPRNG 与 TLS 身份校验**。

```text
┌──────────────────────────────────────────────────────────────────────────────┐
│                         CP 用户层应用 (Application)                           │
├─────────────────────────────┬────────────────────────────┬───────────────────┤
│   非对称密码学 (asymmetric)  │   KDF 与口令哈希 (kdf)      │  令牌与编码 (jwt)  │
│  Ed25519KeyPair             │  HKDF-SHA256 / HKDF-SHA512 │  Jwt (HS256/384/  │
│  X25519KeyPair (ECDH)       │  Scrypt (RFC 7914)         │       512, EdDSA) │
│  EcdsaP256KeyPair (P-256)   │  Argon2id (RFC 9106 / PHC) │  Pem (RFC 7468)   │
├─────────────────────────────┼────────────────────────────┼───────────────────┤
│   扩展哈希与认证 (hash/kdf)  │   AEAD 认证加密 (cipher)   │  CSPRNG & TLS     │
│  SHA-256/384/512, SHA3-256/ │  Aes128Gcm / Aes256Gcm     │  randomBytes/UUID │
│  512, BLAKE3, HMAC-384/512  │  ChaCha20 / XChaCha20-Poly │  TlsHostnameVerif │
└─────────────────────────────┴────────────────────────────┴───────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/crypto/hash.cp` | `// Source: /data/dev/bun/src/boringssl/boringssl.cp` | 单发与流式哈希（`sha256`, `sha512`, `sha1`, `md5`, `sha256Raw`, `Hash`, `createHash`） |
| `lib/crypto/hmac.cp` | `// Source: /data/dev/bun/src/boringssl/boringssl.cp` | 单发与流式消息认证码（`hmacSha256`, `hmacSha256Raw`, `hmacSha512`, `Hmac`, `createHmac`） |
| `lib/crypto/pbkdf2.cp` | `// Source: /data/dev/bun/src/boringssl/boringssl.cp` | PBKDF2-HMAC-SHA256 密钥派生（`pbkdf2Sync`, `pbkdf2Raw`, `pbkdf2Into`）与 `bytesToHex` |
| `lib/crypto/cipher.cp` | `// Source: /data/dev/bun/src/boringssl/boringssl.cp` | AEAD 认证对称加密引擎（`Aes256Gcm`, `ChaCha20Poly1305`） |
| `lib/crypto/random.cp` | `// Source: /data/dev/bun/src/boringssl/boringssl.cp` | 内核级 CSPRNG（`getrandom` 驱动的 `fillRandom`, `randomBytes`, `randomUUID`, `randomUUIDInto`, `randomInt`） |
| `lib/crypto/tls.cp` | `// Source: /data/dev/bun/src/boringssl/boringssl.cp` | RFC 6125 TLS 主机名通配符验证器 `TlsHostnameVerifier` 与 BoringSSL X.509 证书链校验 |
| `lib/crypto/asymmetric.cp` | `// Source: /data/dev/bun/src/boringssl/boringssl.cp & Zig std.crypto` | **非对称密码学**（`Ed25519KeyPair`, `X25519KeyPair`, `EcdsaP256KeyPair`） |
| `lib/crypto/kdf.cp` | `// Source: /data/dev/bun/src/sha_hmac/hmac.cp & Zig std.crypto` | **KDF/口令哈希与现代原语**（`hkdf*`, `scryptDerive*`, `argon2id*`, `Blake3`, `Blake3Stream`, `Sha384`, `Sha3_256`, `Sha3_512`, `HmacSha384`, `HmacSha512`, `XChaCha20Poly1305`, `Aes128Gcm`） |
| `lib/crypto/jwt.cp` | `// Source: Custom implementation by Antigravity AI (RFC 7519 JWT & RFC 7468 PEM)` | **JWT 引擎与 PEM 编解码**（`Jwt`, `JwtParts`, `Pem`, `PemBlock`, `constantTimeEql`, `base64UrlEncode`, `base64UrlDecode`） |
| `lib/crypto/_boringssl.cp` | `// Source: /data/dev/bun/src/boringssl/boringssl.cp` | 底层 BoringSSL C 头文件桥接声明 |
| `lib/crypto/boringssl.cp` | `// Source: /data/dev/bun/src/boringssl/boringssl.cp` | BoringSSL 导出包装层 |
| `lib/crypto/index.cp` | `// Source: /data/dev/bun/src/boringssl/boringssl.cp` | `cap:crypto` 标准库对外统一导出入口 |

---

## 二、 核心密码学原语详解

### 1. 非对称签名与密钥协商 (`lib/crypto/asymmetric.cp`)

- **Ed25519 (`Ed25519KeyPair`)**：
  - `Ed25519KeyPair.generate()`：使用内核 `getrandom` 生成随机 32 字节种子与 RFC 8032 密钥对；
  - `Ed25519KeyPair.fromSeed(seed32)` / `fromSecretKey(sk64)`：从 32 字节种子或 64 字节私钥恢复密钥对；
  - `kp.sign(msg)` / `kp.signInto(&out64, msg)`：生成 64 字节确定性签名；
  - `Ed25519KeyPair.verify(sig64, msg, pub32)` / `verifyStrict(...)`：验证签名，任何篡改立即返回 `false`。
- **X25519 ECDH (`X25519KeyPair`)**：
  - `X25519KeyPair.generate()` / `fromSecret(sk32)`：生成或恢复 32 字节 X25519 密钥对；
  - `X25519KeyPair.sharedSecret(my_sk, peer_pk)` / `kp.computeShared(peer_pk)`：执行 RFC 7748 标量乘法协商 32 字节共享密钥，自动拒绝全零弱点/身份元（返回 `null`）。
- **ECDSA P-256 (`EcdsaP256KeyPair`)**：
  - `EcdsaP256KeyPair.generate()` / `fromSeed(seed32)` / `fromSecret(sk32)`：支持压缩 SEC-1（33B）与非压缩 SEC-1（65B）公钥；
  - `kp.sign(msg)` / `kp.signDer(msg)`：支持 64 字节原始 `(r || s)` 与 ASN.1 DER 签名格式；
  - `EcdsaP256KeyPair.verify(sig64, msg, pk)` / `verifyDer(der_sig, msg, pk)`：同时兼容 33B 压缩与 65B 非压缩 SEC-1 公钥。

### 2. 密钥派生、口令哈希与现代扩展算法 (`lib/crypto/kdf.cp`)

- **HKDF-SHA256 & HKDF-SHA512 (RFC 5869)**：
  - `hkdfExtract(salt, ikm)`、`hkdfExpand(out, info, prk)`、`hkdfDerive(out, ikm, salt, info)`、`hkdfDeriveAlloc(...)`；
  - `hkdfSha512Extract`、`hkdfSha512Expand`、`hkdfSha512Derive`。
- **Scrypt (RFC 7914) & Argon2id (RFC 9106)**：
  - `scryptDerive(out, password, salt, log2_n, r, p)` / `scryptDeriveAlloc(...)`；
  - `argon2idDerive(out, password, salt, t, m, p)`：原始字节派生；
  - `argon2idHash(password, t, m, p)` / `argon2idVerify(encoded_hash, password)`：标准 PHC 格式（`$argon2id$v=19$...`）哈希与校验。
- **现代高速哈希与认证**：
  - **BLAKE3**：`blake3(data)`、`blake3Raw(out, data)`、`Blake3.hash`/`keyed` 与流式 `Blake3Stream.init()` / `initKeyed(key32)`；
  - **SHA-384 / SHA3-256 / SHA3-512**：`sha384`、`sha3_256`、`sha3_512`（及 `*Raw` 零分配版本与同名流式类 `Sha384` / `Sha3_256` / `Sha3_512`）；
  - **HMAC-SHA384 / HMAC-SHA512**：`hmacSha384`、`HmacSha384`、`HmacSha512`（支持单发 `create` 与流式 `init()->update()->digest()`）。
- **AEAD 认证对称加密 (`Aes128Gcm`, `Aes256Gcm`, `ChaCha20Poly1305`, `XChaCha20Poly1305`)**：
  - `XChaCha20Poly1305`：32 字节密钥 + **24 字节扩展 Nonce** + 16 字节 Poly1305 Tag，适合随机 Nonce 场景；
  - `Aes128Gcm`：16 字节密钥 + 12 字节 Nonce + 16 字节 GHASH Tag。

### 3. RFC 7519 JWT 与 RFC 7468 PEM (`lib/crypto/jwt.cp`)

- **JWT (`Jwt` & `JwtParts`)**：
  - 支持 `HS256`、`HS384`、`HS512` 与 `EdDSA`（Ed25519）；
  - 签发：`Jwt.signHs256(payload_json, secret)`、`signHs384`、`signHs512`、`signEdDsa(payload_json, key_pair)`；
  - 校验：`Jwt.verifyHs256(token, secret)`、`verifyHs384`、`verifyHs512`、`verifyEdDsa(token, public_key)`；
  - 安全防线：
    1. 使用 `std.crypto.timing_safe.eql` 执行恒定时间 HMAC 比对；
    2. 解析 Header JSON 并严格强制匹配预期 `alg`，彻底免疫 `alg: "none"` 降级与跨算法混淆；
    3. 返回 `JwtParts?`（包含 `header_json`、`payload_json`、`alg`），任意篡改或格式非法均返回 `null`。
- **PEM (`Pem` & `PemBlock`)**：
  - `Pem.encode(label, der_bytes)`：生成标准 `-----BEGIN <LABEL>-----` 块并严格按 **64 列换行**；
  - `Pem.decode(pem_text)`：校验首尾标签一致性并忽略空白/CRLF 解码 DER 字节，返回 `PemBlock?`。

---

## 三、 完整代码示例

```typescript
import {
    Ed25519KeyPair,
    X25519KeyPair,
    EcdsaP256KeyPair,
    hkdfDerive,
    argon2idHash,
    argon2idVerify,
    Blake3Stream,
    blake3,
    sha3_256,
    XChaCha20Poly1305,
    Jwt,
    Pem,
} from "cap:crypto";

pub fn demoModernCrypto(): !void {
    // 1. Ed25519 签名 & JWT EdDSA
    const kp = Ed25519KeyPair.generate();
    const token = Jwt.signEdDsa("{\"sub\":\"agent-1\"}", kp);
    const verified = Jwt.verifyEdDsa(token, &kp.public_key).?;

    // 2. X25519 ECDH 密钥协商 + HKDF 派生会话密钥
    const alice = X25519KeyPair.generate();
    const bob = X25519KeyPair.generate();
    const shared = alice.computeShared(&bob.public_key).?;
    let session_key: [32]u8 = undefined;
    * = hkdfDerive(&session_key, &shared, "salt", "session-v1");

    // 3. XChaCha20-Poly1305 (24B Nonce) 认证加密
    let nonce24: [24]u8 = [1; 24];
    let ct: [5]u8 = undefined;
    let tag: [16]u8 = undefined;
    XChaCha20Poly1305.encrypt(&session_key, &nonce24, "hello", "aad", &ct, &tag);

    // 4. BLAKE3 流式哈希与 Argon2id 口令存储
    let b3 = Blake3Stream.init();
    b3.update("part1").update("part2");
    const digest = b3.digest();

    const phc = argon2idHash("user-secret", 2, 1024, 1).?;
    const pass_ok = argon2idVerify(phc, "user-secret");

    // 5. PEM 编码/解码
    const pem = Pem.encode("PUBLIC KEY", &kp.public_key);
    const blk = Pem.decode(pem).?;
}
```

---

## 四、 与 `Bun.CryptoHasher` / `node:crypto` API 对照表

| 功能特性 | Node.js / WebCrypto | Bun (`src/boringssl` / `src/sha_hmac`) | CP 标准库 (`cap:crypto`) |
| :--- | :--- | :--- | :--- |
| **SHA-256/384/512 / SHA-3** | `crypto.createHash(algo)` | `new Bun.CryptoHasher(algo)` | `sha256` / `sha384` / `sha512` / `sha3_256` / `sha3_512` |
| **BLAKE3 单发与流式** | 第三方库 | `std.crypto.hash.Blake3` | `blake3(d)` / `Blake3Stream.init()` / `Blake3.keyed` |
| **HMAC-SHA256/384/512** | `crypto.createHmac(algo, key)` | `new Bun.CryptoHasher(algo, key)` | `hmacSha256` / `HmacSha384` / `HmacSha512` |
| **PBKDF2 / HKDF** | `crypto.pbkdf2Sync` / `hkdfSync` | BoringSSL `EVP_PKEY` | `pbkdf2Sync` / `hkdfExtract` / `hkdfExpand` / `hkdfDerive` |
| **Scrypt / Argon2id** | `crypto.scryptSync` | `std.crypto.pwhash` | `scryptDerive` / `argon2idHash` / `argon2idVerify` |
| **AES-128/256-GCM & XChaCha20** | `createCipheriv` / `libsodium` | BoringSSL `EVP_AEAD_CTX` | `Aes128Gcm` / `Aes256Gcm` / `ChaCha20Poly1305` / `XChaCha20Poly1305` |
| **Ed25519 / X25519 / P-256** | `crypto.generateKeyPairSync` | BoringSSL EC/Curve25519 | `Ed25519KeyPair` / `X25519KeyPair` / `EcdsaP256KeyPair` |
| **JWT & PEM** | `jose` / `crypto.KeyObject` | — | `Jwt` (`signHs256/384/512/EdDsa`, `verify*`) / `Pem` (`encode`/`decode`) |
| **CSPRNG & UUIDv4** | `crypto.randomBytes` / `randomUUID` | `crypto.getRandomValues` / `randomUUID` | `randomBytes` / `fillRandom` / `randomUUID` / `randomInt` |
| **TLS 主机名校验** | `tls.checkServerIdentity` | `BoringSSL.checkServerIdentity` | `TlsHostnameVerifier.verify` / `matchDnsName` |

---

## 五、 上游安全边界与性能注意事项

1. **口令哈希（`argon2idHash` / `scryptDerive` / `pbkdf2Sync`）的 CPU 与内存成本**：
   - `argon2idHash` 自动通过内核 `getrandom` 生成随机盐并编码为标准 PHC 字符串（`$argon2id$v=19$m=...,t=...,p=...$<salt>$<hash>`），与 Bun `Bun.password.hash`（Argon2id）格式互通；
   - 口令 KDF 属于计算密集型同步操作，生产高并发服务应放在独立工作线程/Actor 中执行，避免阻塞事件循环主线程。
2. **AEAD Nonce 唯一性红线**：
   - `Aes128Gcm`、`Aes256Gcm`、`ChaCha20Poly1305` 使用 12 字节 Nonce，**同一密钥下严禁重复使用相同 Nonce**（否则会直接泄露认证密钥与明文异或）；
   - 需要无状态随机生成 Nonce 时，优先选用 24 字节扩展 Nonce 的 `XChaCha20Poly1305`。
3. **非对称密钥协商与 JWT 防降级**：
   - `X25519KeyPair.computeShared` / `sharedSecret` 自动检测并拒绝 RFC 7748 低阶点导致的全零共享密钥（返回 `null`）；
   - `Jwt.verify*` 强制执行恒定时间 MAC 比对（`constantTimeEql`）并严格校验 Header 中的 `alg` 字段，彻底阻断 `alg: "none"` 与非对称转对称（RS/EdDSA -> HS）算法混淆攻击。
