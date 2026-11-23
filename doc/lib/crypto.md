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
| `lib/crypto/shaHash.cp` / `shaHmac.cp` / `shaKdf.cp` | `// Reference-hardened against RustCrypto/hashes (MIT/Apache-2.0)` | 统一流式哈希（SHA-1/224/256/384/512/BLAKE2b/2s）、RFC 4231 HMAC（`StreamingHmac`/`hmacInto`/`verifyHmac`）、RFC 5869 HKDF（`hkdf`/`hkdfInto`）与 PBKDF2，内置 16-lane SIMD 常量时间比对 `ctEqualSimd`、`xorKeyPadSimd`、`xorBlockSimd` 与 `secureZeroBytes` |
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
| `lib/crypto/minisign.cp` | `// Origin: Pure-CP Stdlib` | Minisign / OpenBSD Signify 公钥与签名编解码 |
| `lib/crypto/pgp.cp` | `// Origin: Pure-CP Stdlib` | OpenPGP RFC 4880/9580 Armor 与 Packet 解析器（含 CRC24） |
| `lib/crypto/age.cp` | `// Origin: Pure-CP Stdlib` | `age-encryption.org/v1` 文件头、Stanza 与密钥格式解析 |
| `lib/crypto/sshsig.cp` | `// Origin: Pure-CP Stdlib` | OpenSSH `PROTOCOL.sshsig` v1 签名块解析与构造 |
| `lib/crypto/pkcs7.cp` | `// Origin: Pure-CP Stdlib` | RFC 5652 CMS / PKCS#7 SignedData DER 解析器 |
| `lib/crypto/pkcs8.cp` | `// Origin: Pure-CP Stdlib` | RFC 5958 PKCS#8 私钥与 RFC 5280 SPKI 公钥 DER 解析器 |
| `lib/crypto/pkcs12.cp` | `// Origin: Pure-CP Stdlib` | RFC 7292 PKCS#12 PFX 容器 DER 解析器 |
| `lib/crypto/der.cp` | `// Origin: Pure-CP Stdlib` | ITU-T X.690 严格 ASN.1 DER TLV 读写编解码器 |
| `lib/crypto/shamir.cp` | `// Source: cp/lib/shamir` | `GF(2^8)` Shamir 门限秘密分享（`splitSecret`/`combineShares`） |
| `lib/crypto/merkle.cp` | `// Source: cp/lib/merkle` | RFC 6962 Merkle Hash Tree 构建、存在性证明与树差分 |
| `lib/crypto/otp.cp` | `// Source: cp/lib/otp` | RFC 4226 HOTP / RFC 6238 TOTP、Base32 与 `otpauth://` URI |
| `lib/crypto/bech32.cp` | `// Source: cp/lib/bech32` | BIP-173 Bech32 与 BIP-350 Bech32m 地址/字节编解码 |
| `lib/crypto/siphash.cp` | `// Source: cp/lib/siphash` | SipHash-2-4 / SipHash-1-3 与 128-bit 抗哈希洪水 PRF |
| `lib/crypto/fnv.cp` | `// Origin: Pure-CP Stdlib` | FNV-1/FNV-1a (32/64/128-bit)、DJB2、SDBM、Jenkins OAAT |
| `lib/crypto/murmur3.cp` | `// Source: cp/lib/murmur3` | MurmurHash3 `x86_32` 与 `x64_128`（单发 + 流式） |
| `lib/crypto/crc.cp` | `// Source: cp/lib/crc` | CRC-8/16/32 (IEEE & Castagnoli)/64（含 `crc32Combine`） |
| `lib/crypto/simhash.cp` | `// Origin: Pure-CP Stdlib` | 64-bit Charikar SimHash、汉明相似度与 4x16-bit LSH 去重 |
| `lib/crypto/minhash.cp` | `// Origin: Pure-CP Stdlib` | MinHash 集合 Jaccard 相似度、LSH Band 与基数估算 |
| `lib/crypto/index.cp` | `// Source: /data/dev/bun/src/boringssl/boringssl.cp` | `cap:crypto` 标准库对外统一零冲突导出入口（共 315 符号） |

### 2. 归档参考实现（Reference Only，不从 `cap:crypto` 导出）

以下纯 CP 参考实现与上述权威模块存在功能重叠，已按 `*_ .cp` 约定归档于 `lib/crypto/` 供对比分析（不从 `lib/crypto/index.cp` 重导出，生产代码请统一使用 `cap:crypto` 权威 API）：

| 归档文件 | 对应权威模块 | 说明 |
| :--- | :--- | :--- |
| `lib/crypto/aes_gcm_.cp` | `cipher.cp` / `kdf.cp` | AES-128/256-GCM 与 AES-256-GCM-SIV 纯 CP 参考实现 |
| `lib/crypto/argon2_.cp` | `kdf.cp` | RFC 9106 Argon2d/2i/2id 与 PHC 编解码参考实现 |
| `lib/crypto/blake3_.cp` | `kdf.cp` | 纯 CP BLAKE3 压缩/Merkle/XOF 流式参考实现 |
| `lib/crypto/chacha20_.cp` | `cipher.cp` / `kdf.cp` | RFC 8439 ChaCha20-Poly1305 / XChaCha20-Poly1305 参考实现 |
| `lib/crypto/ed25519_.cp` | `asymmetric.cp` | RFC 8032 Ed25519 签名与批验证参考实现 |
| `lib/crypto/hkdf_kdf_.cp` | `kdf.cp` | RFC 5869 HKDF-SHA256/384/512 Extract & Expand 参考实现 |
| `lib/crypto/scrypt_.cp` | `kdf.cp` | RFC 7914 scrypt 与 PHC 编解码参考实现 |
| `lib/crypto/x25519_.cp` | `asymmetric.cp` | RFC 7748 X25519 ECDH 密钥协商参考实现 |

> 注：Java KeyStore (`.jks`) 与 TCG TPM 2.0 二进制结构解析器已统一收敛归属至 `cap:binfmt`（`lib/binfmt/jks.cp`、`lib/binfmt/tpm.cp`）。

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

---

## 六、 多版本密钥环轮换与信封 MAC 校验器 (`keyring.cp`)

`lib/crypto/keyring.cp` 提供 100% 纯 CP、零堆分配的多版本密钥环（`KeyRing`），内置 RFC 6234 SHA-256 / RFC 2104 HMAC-SHA256 与恒定时间比较，专为微服务凭证与 AI Agent 令牌零停机轮换设计：

- **平滑零停机轮换**：`addKey(..., make_active = true, ...)` 自动将旧 `Active` 密钥降级为 `VerifyOnly`，宽限期内同时支持新旧版本验证；
- **安全治理防线**：自动拦截 `Revoked` 与已过期（`now_ms >= expires_ms`）密钥，撤销与清空时主动清零内存中的密钥材料，MAC 计算域绑定 `kid` 防止跨版本混淆。

```cp
import { KeyRingEntry, KeyRing } from "cap:crypto";

pub fn demoKeyRingRotation() {
    let slots: [4]KeyRingEntry = [KeyRingEntry{}; 4];
    let ring = KeyRing.init(slots[0..]);

    const k1: [32]u8 = [0x11; 32];
    const k2: [32]u8 = [0x22; 32];
    * = ring.addKey(1, k1, true, 1000, 10000);

    let tag1: [32]u8 = [0; 32];
    const kid1 = ring.signEnvelope("agent-token-v1", 2000, tag1[0..]) ?? return;

    // 零停机轮换至 v2（v1 自动降级为 VerifyOnly，旧令牌在宽限期内仍可通过验证）
    * = ring.addKey(2, k2, true, 3000, 20000);
    const old_still_valid = ring.verifyEnvelope(kid1, "agent-token-v1", tag1[0..], 4000);
    * = old_still_valid; // true
}
```

---

## 七、 零分配 RFC 6962 域分离 Merkle Tree 与包含性证明引擎 (`merkleTree.cp`)

面向透明审计日志、包管理器构件校验、AI Agent 执行轨迹存证与分布式快照一致性的 100% 纯 CP 零动态堆分配 RFC 6962 SHA-256 二叉 Merkle Tree（规避与 Allocator 版 `merkle.cp::MerkleTree` 冲突，导出为 `Rfc6962MerkleTree`）：

- **常量与模型**：`MERKLE_HASH_LEN` (`32`)、`MERKLE_MAX_LEAVES` (`128`)、`MERKLE_MAX_PROOF_DEPTH` (`8`)、`MerkleHash`（内置常量时间 `eql`）、`MerkleProofStep`、`MerkleInclusionProof`。
- **域分离哈希原语（防第二原像混淆）**：
  - `merkleHashEmpty(): [32]u8` -> `SHA256("")`
  - `merkleHashLeaf(data): [32]u8` -> `SHA256(0x00 || data)`
  - `merkleHashNode(left, right): [32]u8` -> `SHA256(0x01 || left || right)`
- **零堆构树与证明生成 (`Rfc6962MerkleTree`)**：
  - `init()`、`append_leaf(data)`、`append_leaf_hash(h)`、`leaf_count()`、`compute_root()`（按 RFC 6962 `largestPowerOfTwoLessThan(n)` 规范切分，支持 0..128 任意非 2 次幂叶子数）、`generate_inclusion_proof(leaf_index): MerkleInclusionProof?`。
- **独立无状态常量时间验证器**：
  - `verifyMerkleInclusionProof(leaf_data, proof, expected_root): bool`（严格校验路径深度、`sibling_is_left` 与 `(leaf_index, leaf_count)` 结构一致性，并以常量时间比对根哈希）。

```cp
import {
    Rfc6962MerkleTree,
    verifyMerkleInclusionProof,
} from "cap:crypto";

pub fn demoRfc6962Merkle() {
    let tree = Rfc6962MerkleTree.init();
    * = tree.append_leaf("step-1:tool-call");
    * = tree.append_leaf("step-2:rag-query");
    * = tree.append_leaf("step-3:final-answer");

    const root = tree.compute_root();
    if (proof := tree.generate_inclusion_proof(1)) {
        const ok = verifyMerkleInclusionProof("step-2:rag-query", proof, root);
        * = ok; // true
    }
}
```

### 零堆分配前向安全对称密钥棘轮与乱序跳步密钥缓存引擎 (`ratchet.cp`)

面向安全多 Agent 通信通道、TLS 1.3 密钥更新与 Signal 风格前向安全（Forward Secrecy）的 100% 零动态堆分配对称密钥棘轮内核（基于 RFC 5869 HKDF-SHA256 与 HMAC-SHA256）：

- **常量与数据模型**：
  - `RATCHET_KEY_LEN` (`32`)、`RATCHET_MAX_SKIPPED_KEYS` (`32`)
  - `RatchetDerivedKey`（`step_index: u64`, `key: [32]u8`, `nonce_prefix: [12]u8`）
  - `RatchetStats`（`current_step`, `cached_skipped_keys`, `total_derived`, `total_out_of_order_hits`, `rekey_count`）
- **核心状态机 `SymmetricKeyRatchet`**：
  - `init(root_secret, salt, context_info): SymmetricKeyRatchet?`：基于 HKDF-SHA256 `Extract + Expand` 派生初始 32B `chain_key`（拒绝空 `root_secret`）。
  - `next_send_key(*_): RatchetDerivedKey`：单向推进 `msg_key = HMAC(ck, 0x01)`、`nonce_prefix = HMAC(ck, 0x03)[0..12]`、`chain_key = HMAC(ck, 0x02)`，旧链密钥即刻覆写消除。
  - `receive_key_for_step(*_, target_step: u64): RatchetDerivedKey?`：支持顺序与乱序到达；前向跳步受 `<= 32` 与空槽容量原子防资源耗尽守卫；历史跳步密钥一次性消费并立即清零，杜绝重放。
  - `rekey(*_, new_entropy, info): bool`：通过 HKDF 混合新熵实现 Post-Compromise Security（拒绝空熵）。
  - `zeroize(*_)`：安全清零链密钥与全部缓存跳步密钥。
  - `stats(): RatchetStats`。

```cp
import {
    SymmetricKeyRatchet,
} from "cap:crypto";

pub fn demoSymmetricKeyRatchet() {
    let tx = SymmetricKeyRatchet.init("shared-root-secret", "session-salt", "a2a-chan-v1") ?? return;
    let rx = SymmetricKeyRatchet.init("shared-root-secret", "session-salt", "a2a-chan-v1") ?? return;

    const k0 = tx.next_send_key();
    const k1 = tx.next_send_key();
    const k2 = tx.next_send_key();

    // 乱序到达：先收 step 2，再收 step 0、step 1（每步密钥精确匹配且仅可消费一次）
    * = rx.receive_key_for_step(2);
    * = rx.receive_key_for_step(0);
    * = rx.receive_key_for_step(1);
    * = k0; * = k1; * = k2;

    tx.zeroize();
    rx.zeroize();
}
```

---

### 16. 零分配版本化信封加密与密钥轮换环（`envelope.cp`）

纯 CP 零动态堆分配、零 C FFI 依赖的版本化信封加密头编解码、多版本 KEK 轮换密钥环与 Encrypt-then-MAC 端到端加解密引擎：

- **紧凑二进制信封头（`CPE1`）**：`encodeEnvelopeHeader` / `decodeEnvelopeHeader` 确定性编解码 `Magic("CPE1") | version(u8) | alg_id(u8) | key_version(u32 BE) | nonce_len(u8) | salt_len(u8) | aad_len(u32 BE) | ct_len(u32 BE) | nonce | salt | tag(32B)`。
- **多版本密钥环（`EnvelopeKeyring`）**：内联 16 槽位，支持 `primary | active_decrypt_only | retired | revoked` 四态生命周期、`registerKey`、`promotePrimary`（原子轮换并降级旧主密钥）、`retireKey`、`revokeKey`（立即覆写清零 KEK）、`selectEncryptKey`、`resolveDecryptKey`、`needsRewrap(key_version)`、`stats` 与 `zeroize`。
- **端到端零堆 `sealEnvelope` / `openEnvelope`**：基于 RFC 5869 HKDF-SHA256 按 `salt + alg_id + key_version + nonce` 派生独立 `enc_key` 与 `mac_key`，通过流式 HMAC-SHA256-CTR 加密与常数时间 Encrypt-then-MAC（绑定完整信封头 + AAD + 密文）防护截断、AAD/标签/密文篡改及已吊销密钥解密。

```cp
import {
    EnvelopeStatus,
    EnvelopeKeyState,
    EnvelopeKeyring,
    sealEnvelope,
    openEnvelope,
} from "cap:crypto";

pub fn demoEnvelopeSeal() {
    let ring = EnvelopeKeyring.init();
    const kek_v1: [32]u8 = [0x11; 32];
    const kek_v2: [32]u8 = [0x22; 32];
    * = ring.registerKey(1, kek_v1, EnvelopeKeyState.primary, 1000);

    let env_buf: [256]u8 = [0; 256];
    let env_slice: []u8 = env_buf[0..];
    const sealed = sealEnvelope(ring, "nonce_0123456789", "salt_0123456789a", "tenant:acme", "secret payload", *env_slice);
    if (sealed.status != EnvelopeStatus.ok) return;

    // 密钥轮换至 v2：v1 自动降级为 active_decrypt_only，旧密文仍可解密且触发 needs_rewrap == true
    * = ring.registerKey(2, kek_v2, EnvelopeKeyState.primary, 2000);
    let pt_buf: [128]u8 = [0; 128];
    let pt_slice: []u8 = pt_buf[0..];
    const opened = openEnvelope(ring, env_buf[0..sealed.bytes_written], "tenant:acme", *pt_slice);
    * = opened.needs_rewrap; // true -> 触发后台重加密迁移至 v2
    ring.zeroize();
}
```

### 零分配 GF(256) Shamir `(k, n)` 门限密钥分片与拉格朗日插值重构引擎 (`shamirSecret.cp`)

面向密钥托管、分布式钱包与零信任秘密恢复的 100% 纯 CP 零动态堆分配、零 C FFI 依赖 Shamir `(k, n)` 门限秘密共享引擎：

- **常量时间 GF(256) 有限域原语（AES 不可约多项式 `0x11B`）**：
  - `gf256Mul(a, b)`（无分支常数时间乘法）、`gf256Inv(a)`（无查表 `a^254` 常数时间求逆）、`gf256Div(a, b): u8?`（除零返回 `null`）。
- **常量、状态与数据结构**：
  - `SHAMIR_MAX_SECRET_LEN = 64`、`SHAMIR_MIN_THRESHOLD = 2`、`SHAMIR_MAX_SHARES = 32`、`SHAMIR_TAG_LEN = 32`
  - `ShamirStatus`（`ok | invalid_threshold | invalid_share_count | invalid_secret_len | insufficient_entropy | insufficient_shares | invalid_x_zero | duplicate_x | mismatched_share_len | output_too_small | tag_mismatch`）
  - `ShamirShare`（`x`, `threshold`, `secret_len`, `has_tag`, `share_tag`, `y`, `payloadSlice()`）、`ShamirSplitResult`、`ShamirCombineResult`
- **核心纯函数**：
  - `shamirRequiredEntropyBytes(secret_len, threshold)`：计算外部熵所需精确字节数 `secret_len * (threshold - 1)`。
  - `splitSecretWithEntropy(secret, k, n, entropy_bytes, attach_tag, out_shares)` / `splitSecretDeterministic(secret, k, n, rng_seed, attach_tag, out_shares)`：Horner 多项式求值拆分 `1..64B` 秘密为 `2 <= k <= n <= 32` 份分片。
  - `combineShamirShares(shares, out_secret)`：任意 `>= k` 份不同非零坐标分片在 `x = 0` 处拉格朗日插值无损重构；若分片携带 SHA-256 秘密承诺摘要则自动常数时间校验并拦截篡改分片（`tag_mismatch`）。
  - `computeSecretTagSha256(secret)` / `verifyReconstructedSecret(secret, expected_tag)`：域分离 SHA-256 承诺摘要与常数时间比对。

```cp
import {
    ShamirStatus,
    ShamirShare,
    splitSecretDeterministic,
    combineShamirShares,
} from "cap:crypto";

pub fn demoShamirSecretSharing() {
    const secret: []const u8 = "master-vault-key-2026";
    let shares: [5]ShamirShare = [ShamirShare{}; 5];
    let shares_s: []ShamirShare = shares[0..];
    * = splitSecretDeterministic(secret, 3, 5, 4242, true, *shares_s);

    // 任意 3 份分片无损重构并自动校验 SHA-256 承诺摘要
    const subset: [3]ShamirShare = [shares[4], shares[0], shares[2]];
    let out: [32]u8 = [0; 32];
    let out_s: []u8 = out[0..];
    const res = combineShamirShares(subset[0..], *out_s);
    * = (res.status == ShamirStatus.ok && res.tag_verified); // true
}
```

---

## 零分配稀疏默克尔树（Sparse Merkle Tree，`sparseMerkle.cp`）

面向可验证键值存储、零知识状态承诺与撤销列表的 100% 纯 CP 零动态堆分配、零 C FFI 依赖稀疏默克尔树引擎（基于 `std.crypto.hash.sha2.Sha256` 严格域分离 `0x00` 叶子 / `0x01` 内部节点）：

- **常量与原语**：
  - `SMT_HASH_LEN = 32`、`SMT_MAX_DEPTH = 32`、`SMT_MAX_LEAVES = 64`
  - `smtEmptyLeafHash()`（32 字节全零空叶，与 `smtHashLeaf("") = SHA256(0x00)` 密码学可区分）、`smtHashLeaf(value_bytes)`、`smtHashNode(left, right)`
- **数据模型**：
  - `SmtLeafEntry`、`SmtProof`（`depth`, `key`, `leaf_present`, `value_hash`, `siblings[32][32]`）、`SmtStats`
- **核心引擎 `SparseMerkleTree`**：
  - `init(depth: u8): SparseMerkleTree?`：支持 `depth ∈ 1..=32` 并预计算 `default_hashes[0..=depth]`（零存储全空子树）。
  - `put(*_, key, value_bytes): bool` / `delete(*_, key): bool` / `contains(key)` / `getLeafHash(key): ([32]u8)?` / `clear(*_)`：插入/原位更新与幂等删除（删除自动剪枝回退默认空哈希）。
  - `rootHash(): [32]u8` / `defaultRootHash(): [32]u8` / `generateProof(key): SmtProof?` / `stats(): SmtStats`。
- **无状态常数时间验证器**：
  - `verifySmtInclusionProof(root, key, value_bytes, proof): bool`：验证存在性证明。
  - `verifySmtNonInclusionProof(root, key, proof): bool`：验证非存在性证明（任意 sibling/root/value/key 篡改严格拒绝）。

```cp
import {
    SparseMerkleTree,
    verifySmtInclusionProof,
    verifySmtNonInclusionProof,
} from "cap:crypto";

pub fn demoSparseMerkleTree() {
    let tree = SparseMerkleTree.init(16) ?? return;
    * = tree.put(42, "verified-state");
    const root = tree.rootHash();

    if (inc := tree.generateProof(42)) {
        * = verifySmtInclusionProof(root, 42, "verified-state", inc); // true
    }
    if (non_inc := tree.generateProof(99)) {
        * = verifySmtNonInclusionProof(root, 99, non_inc); // true
    }
}
```

---

## 零分配 Macaroon 能力令牌与链式 HMAC 衰减引擎 (`macaroon.cp`)

面向微服务零信任授权、去中心化能力委托（Capability Delegation）与 AI Agent 细粒度沙箱权限衰减的 100% 纯 CP 零动态堆分配 **Macaroons (Birgisson et al. 2014)** 引擎：

- **常量与枚举**：
  - `MACAROON_SIG_LEN = 32`、`MACAROON_MAX_LOCATION_LEN = 64`、`MACAROON_MAX_ID_LEN = 64`、`MACAROON_MAX_CAVEATS = 16`、`MACAROON_MAX_CAVEAT_LEN = 96`、`MACAROON_MAX_CUSTOM_CLAIMS = 8`
  - `MacaroonStatus`（`ok | invalid_root_key | invalid_identifier | location_too_long | identifier_too_long | caveat_empty | caveat_too_long | caveats_full | signature_mismatch | caveat_expired | caveat_scope_denied | caveat_method_denied | caveat_prefix_denied | caveat_assertion_failed | caveat_malformed | buffer_too_small | invalid_format | truncated`）
- **核心引擎 `Macaroon`**：
  - `mint(root_key, location, identifier): Macaroon?`：生成根能力令牌 `sig_0 = HMAC-SHA256(root_key, identifier)`。
  - `addCaveat(*_, caveat)` / `addExpCaveat(*_, exp_ts)` / `addScopeCaveat(*_, scope)` / `addMethodCaveat(*_, method)` / `addPrefixCaveat(*_, path_prefix)`：无需 `root_key` 的离线单向能力衰减 `sig_i = HMAC-SHA256(sig_{i-1}, caveat_i)`，数学上不可逆去衰减。
  - `verifySignature(root_key): bool`：常量时间链式 HMAC-SHA256 签名验证（内置 `macaroonConstantTimeEq` 防侧信道）。
  - `verify(root_key, ctx: MacaroonVerifyContext): MacaroonVerifyResult`：Fail-Closed 校验签名与全部一阶 Caveat（支持 `exp <`、`exp <=`、`scope =`、`method =`、`prefix =` 及自定义 `key = value` 精确/通配 `*` 断言），精确返回失败原因与违规 `failed_caveat_index`。
  - `encodedSize()` / `encodeBinary(*out_buf): usize?` / `decodeBinary(data): Macaroon?`：零堆分配 `MAC1` 二进制线协议编解码，严格拦截截断、越界与尾随垃圾字节。

```cp
import {
    Macaroon,
    MacaroonVerifyContext,
} from "cap:crypto";

pub fn demoMacaroon() {
    const root_key: []const u8 = "kms-root-secret-2026";
    let tok = Macaroon.mint(root_key, "https://api.cap.dev", "key-id-42") ?? return;
    // 离线逐级衰减（委托方无需持有 root_key）
    * = tok.addExpCaveat(2000);
    * = tok.addScopeCaveat("agent:read");
    * = tok.addMethodCaveat("GET");
    * = tok.addPrefixCaveat("/v1/docs/");
    * = tok.addCaveat("tenant = acme-*");

    let ctx = MacaroonVerifyContext.init(1500, "agent:read", "GET", "/v1/docs/spec.md");
    * = ctx.addClaim("tenant", "acme-prod");
    const res = tok.verify(root_key, ctx);
    * = res.isOk(); // true
}
```

---

### 零分配 Merkle Mountain Range (MMR) 仅追加密码学累加器 (`merkleMmr.cp`)

面向不可篡改审计日志、轻客户端状态承诺与区块链历史前缀校验的 100% 纯 CP 零动态堆分配 Merkle Mountain Range (MMR) 引擎：

- **常量与域分离 SHA-256 原语**：
  - `MMR_HASH_LEN = 32`、`MMR_MAX_LEAVES = 128`、`MMR_MAX_NODES = 256`、`MMR_MAX_HEIGHT = 8`、`MMR_MAX_PEAKS = 16`
  - `mmrHashEmpty()` (`SHA256("")`)、`mmrHashLeaf(pos, payload)` (`SHA256(0x00 || pos_le64 || payload)`)、`mmrHashParent(pos, left, right)` (`SHA256(0x01 || pos_le64 || left || right)`)、`mmrHashBag(mmr_size, left_peak, right_bag)` (`SHA256(0x02 || mmr_size_le64 || left_peak || right_bag)`)、`mmrConstantTimeEq(a, b)`
- **数据模型与独立验证器**：
  - `MmrPeak`（`pos`, `height`, `hash`）、`MmrInclusionProof`、`MmrStats`（`leaf_count`, `node_count`, `peak_count`, `max_height`）
  - `verifyMmrProof(root, proof, leaf_data): bool`：无状态常量时间 O(log N) 包含性证明验证（沿子树兄弟爬升至对应山峰，再完成左右山峰 bagging）。
- **核心引擎 `MerkleMmr`**：
  - `init()` / `clear(*_)` / `leafCount()` / `nodeCount()` / `peakCount()` / `stats()`
  - `appendLeaf(*_, data): u64?`：O(log N) 仅追加叶子并自动合并同高度相邻山峰，返回 0-based MMR 节点位置。
  - `bagPeaks(): [32]u8` / `peaks(*out_peaks): usize`：从右向左折叠全部山峰输出单一 32 字节根哈希，或提取全部山峰位置、高度与哈希。
  - `generateProof(leaf_index): MmrInclusionProof?` / `rootAtLeafCount(prefix_leaf_count): ([32]u8)?` / `verifyPrefixRoot(prefix_leaf_count, expected_root): bool`：O(log N) 证明生成与历史前缀快照一致性验证。

```cp
import {
    MerkleMmr,
    verifyMmrProof,
} from "cap:crypto";

pub fn demoMerkleMmr() {
    let mmr = MerkleMmr.init();
    * = mmr.appendLeaf("tx:0");
    * = mmr.appendLeaf("tx:1");
    * = mmr.appendLeaf("tx:2");
    const root3 = mmr.bagPeaks();

    if (proof := mmr.generateProof(1)) {
        * = verifyMmrProof(root3, proof, "tx:1"); // true
    }
    * = mmr.appendLeaf("tx:3");
    * = mmr.verifyPrefixRoot(3, root3); // true (append-only 历史前缀不可篡改!)
}
```

---

### 零分配 TLS 公钥指纹钉扎 SPKI Pinning 与备用 Pin 轮换校验引擎 (`certPin.cp`)

面向高安全 TLS 连接校验的 100% 纯 CP 零动态堆分配 SPKI SHA-256 公钥指纹钉扎与轮换引擎（RFC 7469 风格）：

- **常量与枚举**：
  - `CERT_PIN_MAX_POLICIES = 32`、`CERT_PIN_MAX_PINS_PER_HOST = 8`、`CERT_PIN_HASH_LEN = 32`、`CERT_PIN_HOST_MAX_LEN = 128`、`CERT_PIN_B64_LEN = 44`、`CERT_PIN_DIRECTIVE_LEN = 57`
  - `CertPinMode`（`enforce | report_only`）
  - `CertPinOpStatus`（`ok | invalid_host | invalid_pin_list | backup_pin_required | duplicate_pin | host_exists | not_found | capacity_full`）
  - `CertPinVerifyStatus`（`matched | no_pin_policy | pin_expired | mismatch_blocked | mismatch_reported | invalid_input`）
- **纯函数工具**：
  - `certPinConstantTimeEq(a, b): bool`：32 字节常量时间指纹比对（杜绝短路计时侧信道）。
  - `encodePinSha256Base64(hash, out_buf)` / `decodePinSha256Base64(b64)`：零分配规范 RFC 4648 Base64 编解码（拒绝非规范尾部填充位）。
  - `formatPinDirective(hash, out_buf)` / `parsePinDirective(directive)`：零分配 `pin-sha256="<44B>"` 指令格式化与解析。
- **核心引擎 `CertPinStore`**：
  - `init()` / `clear(*_)` / `addPolicy(*_, host, pins, include_subdomains, require_backup_pin, mode, expires_epoch_s)` / `rotatePins(*_, host, new_pins, new_expires_epoch_s)` / `addPin` / `removePin` / `removePolicy` / `findExactHost` / `getPolicy`
  - 安全轮换守卫：`require_backup_pin == true` 时强制至少 2 个互不重复的非零 Pin 才允许激活或移除旧 Pin，防止单 Pin 轮换锁死服务。
  - `verifyChain(*_, host, chain_spki_hashes, now_epoch_s): CertPinDecision`：规范化域名（大小写无关、剥离根点），精确主机优先于 `include_subdomains` 最长父域继承（拒绝 `evil-example.com` 误匹配 `example.com`），常量时间全链比对并返回精准裁决。
  - `stats(): CertPinStats` / `resetStats(*_)`

```cp
import {
    CertPinMode,
    CertPinVerifyStatus,
    CertPinStore,
    formatPinDirective,
} from "cap:crypto";

pub fn demoCertPinning() {
    let store = CertPinStore.init();
    const pin_primary: [32]u8 = [0x11; 32];
    const pin_backup: [32]u8 = [0x22; 32];
    const pins: [2][32]u8 = [pin_primary, pin_backup];

    * = store.addPolicy("api.cap.dev", pins[0..], true, true, CertPinMode.enforce, 1_800_000_000);

    const chain: [2][32]u8 = [[0x99; 32], pin_backup];
    const dec = store.verifyChain("v1.api.cap.dev", chain[0..], 1_700_000_000);
    * = (dec.status == CertPinVerifyStatus.matched);

    let dir_buf: [64]u8 = [0; 64];
    let dir_s: []u8 = dir_buf[0..];
    * = formatPinDirective(pin_primary, *dir_s);
}
```

---

### 零分配 TLS 1.3 / QUIC RFC 9001 AEAD Nonce 派生、防重用守卫与 128 位防重放滑窗 (`aeadNonce.cp`)

面向 AES-GCM / ChaCha20-Poly1305 安全传输层的 100% 纯 CP 零动态堆分配 Nonce 派生与 128 位防重放引擎：

- **常量与纯函数**：
  - `AEAD_NONCE_LEN = 12`、`AEAD_ANTI_REPLAY_WINDOW_BITS = 128`、`AEAD_DEFAULT_REKEY_LIMIT = 8_388_608`
  - `deriveAeadNonce(base_iv: [12]u8, seq: u64): [12]u8`：按 RFC 9001 §5.3 / TLS 1.3 §5.3 将大端 64 位 `seq` 与 `base_iv[4..12]` 异或。
- **枚举与数据模型**：
  - `AntiReplayVerdict`（`accepted | duplicate_replay | too_old_below_window`）
  - `AeadNonceTicket`（`seq`, `key_epoch`, `nonce`）、`AeadNonceSenderStats`、`AntiReplayStats`
- **发送端防重用守卫 `AeadNonceSender`**：
  - `init(base_iv, rekey_limit, warn_threshold): AeadNonceSender?` / `initDefault(base_iv)`
  - `nextNonce(*_): AeadNonceTicket?`：严格单调递增序号，到达 `rekey_limit` 硬上限后拒绝签发返回 `null`，彻底杜绝 Nonce 重用灾难。
  - `needsRekey()` / `isExhausted()` / `remaining()` / `rekey(*_, new_base_iv)`（递增 `key_epoch` 并归零序号）/ `stats()`
- **接收端 128 位防重放滑窗 `AntiReplayWindow128` & `AeadNonceReceiver`**：
  - `checkOnly(seq)`（AEAD 验签前只读预检，防止伪造包污染窗口）/ `checkAndMark(*_, seq)`（验签通过后 O(1) 标记，支持最高 128 包乱序到达与大序号跳跃清窗）/ `rekey(*_)` / `stats()`。

```cp
import {
    AeadNonceSender,
    AeadNonceReceiver,
    AntiReplayVerdict,
} from "cap:crypto";

pub fn demoAeadNonce() {
    const iv: [12]u8 = [0x01, 0x02, 0x03, 0x04, 0xAA, 0xBB, 0xCC, 0xDD, 0x10, 0x20, 0x30, 0x40];
    let tx = AeadNonceSender.init(iv, 1000, 800) ?? return;
    let rx = AeadNonceReceiver.init(iv);

    if (ticket := tx.nextNonce()) {
        * = rx.deriveForSeq(ticket.seq); // 与 ticket.nonce 字节逐一相同
        * = (rx.checkAndMark(ticket.seq) == AntiReplayVerdict.accepted);
        * = (rx.checkAndMark(ticket.seq) == AntiReplayVerdict.duplicate_replay);
    }
}
```

---

## X.509 Subject Alternative Name 安全校验与防注入序列化器（`x509AltName.cp`）

吸收自 `/data/dev/bun/src/runtime/api/bun/x509.cp`，提供 100% 纯 CP、零动态堆分配的 RFC 5280 SAN 安全校验、解析与防分隔符注入格式化能力：

- **导出符号**：`X509_ALT_NAME_MAX_ENTRIES` (`32`)、`X509AltNameKind`（`Dns` / `Ip` / `Email` / `Uri` / `DirName` / `RegisteredId`）、`X509AltNameError`（`BufferTooSmall` / `UnsafeAltName` / `InvalidEntry` / `UnknownPrefix`）、`isSafeAltName`、`X509AltNameEntry`（`init` / `initUtf8` / `isSafe` / `formatStrictInto` / `formatInto`）、`parseX509AltNameEntry`、`formatX509AltNameList`、`countSafeX509AltNames`；
- **安全边界**：`isSafeAltName(name, utf8)` 严格拦截 `"`、`\`、`,`、`'` 及控制字符（`< 32` / `127`，ASCII 模式同时拦截 `> 126`）；`formatInto` 对包含逗号或引号的畸形 SAN 值自动包裹引号并十六进制转义，阻断逗号分隔 `subjectaltname` 伪造。

```cp
import { isSafeAltName, X509AltNameEntry, X509AltNameKind, formatX509AltNameList } from "cap:crypto";

const ok = isSafeAltName("api.example.com", false); // true
const entries: [2]X509AltNameEntry = [
    X509AltNameEntry.init(X509AltNameKind.Dns, "api.example.com"),
    X509AltNameEntry.init(X509AltNameKind.Ip, "127.0.0.1"),
];
let out: [128]u8 = [0; 128];
const n = formatX509AltNameList(entries[0..], out[0..])?; // "DNS:api.example.com, IP Address:127.0.0.1"
* = ok; * = n;
```

---

## 收敛子模块深度文档

- [aes_gcm.md](crypto/aes_gcm.md)
- [age.md](crypto/age.md)
- [argon2.md](crypto/argon2.md)
- [bech32.md](crypto/bech32.md)
- [blake3.md](crypto/blake3.md)
- [chacha20.md](crypto/chacha20.md)
- [crc.md](crypto/crc.md)
- [der.md](crypto/der.md)
- [ed25519.md](crypto/ed25519.md)
- [fnv.md](crypto/fnv.md)
- [hkdf_kdf.md](crypto/hkdf_kdf.md)
- [merkle.md](crypto/merkle.md)
- [minhash.md](crypto/minhash.md)
- [minisign.md](crypto/minisign.md)
- [murmur3.md](crypto/murmur3.md)
- [otp.md](crypto/otp.md)
- [pgp.md](crypto/pgp.md)
- [pkcs12.md](crypto/pkcs12.md)
- [pkcs7.md](crypto/pkcs7.md)
- [pkcs8.md](crypto/pkcs8.md)
- [scrypt.md](crypto/scrypt.md)
- [shamir.md](crypto/shamir.md)
- [simhash.md](crypto/simhash.md)
- [siphash.md](crypto/siphash.md)
- [sshsig.md](crypto/sshsig.md)
- [x25519.md](crypto/x25519.md)

## 历史密码学域散装模块收敛（W29-Sync R7）

- `cap:sha_hmac` -> `lib/crypto/sha{Hash,Hmac,Kdf}.cp`：导出 `ShaError`, `HashAlg`, `MAX_DIGEST_LEN`, `digestLen`, `blockLen`, `StreamingHasher`, `digestHex`, `hashOneshot`, `hashHex`, `StreamingHmac`, `hmacOneshot`, `hmacHex`, `verifyHmac`, `shaHkdfExtract`, `shaHkdfExpand`, `hkdf`, `pbkdf2HmacSha256`, `pbkdf2Hmac`。
