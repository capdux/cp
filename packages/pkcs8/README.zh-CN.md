[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# pkcs8

> **源包溯源**：吸收自 **crates.io** [`pkcs8`](https://github.com/RustCrypto/formats)（`MIT`），CP 官方洁净室实现 `pkcs8@1.0.0`。

纯 CP 实现的零堆分配 RFC 5958 PKCS#8 `PrivateKeyInfo`（v1）/ `OneAsymmetricKey`（v2）及 RFC 8410 Ed25519 / P-256 DER & PEM 编解码器。底层复用 `spki`、`const-oid`、`pem-rfc7468` 与 `subtle`。

## 核心亮点

- **零堆分配**：内置固定长度密钥缓冲（`private_key: [160]u8`、`public_key: [64]u8`），覆盖 Ed25519、P-256 等主流算法，全程零堆分配。
- **敏感密钥安全擦除**：`PrivateKeyInfo.zeroize()` 调用 `subtle` 的 `zeroizeBytes`，可靠清零私钥与公钥缓冲。
- **零栈清零优化**：内部固定栈缓冲采用 `undefined` 初始化，消除热路径多余栈清零。
- **完整 RFC 5958 / 8410 支持**：内置 `PrivateKeyInfo.ed25519(seed32)`、`PrivateKeyInfo.ed25519V2(seed32, pub32)`（含 `[1]` 公钥字段）及 `PrivateKeyInfo.p256(ec_priv_der)` 快捷构造器。

## 零成本迁移（Rust → CP）

直接沿用原包名 `pkcs8`，无需改名：

**Rust：**
```rust
use pkcs8::{PrivateKeyInfo, DecodePrivateKey, EncodePrivateKey};
```

**CP：**
```typescript
import {
    PrivateKeyInfo, Pkcs8DecodeResult,
    encodePkcs8Der, decodePkcs8Der, encodePkcs8Pem, decodePkcs8Pem
} from "pkcs8";
```

## 实战示例

### 1. 将 Ed25519 种子编码为 PKCS#8 PEM 并自动擦除

```typescript
import { PrivateKeyInfo, encodePkcs8Pem } from "pkcs8";

export fn exportEd25519Pem(seed32: &[]u8, pem_out: []u8): i64 {
    let mut built = PrivateKeyInfo.ed25519(seed32);
    if (!built.ok) {
        return built.err;
    }
    defer built.info.zeroize(); // 退出时安全擦除私钥字节

    return encodePkcs8Pem(built.info, false, pem_out);
}
```

### 2. 解码 PKCS#8 PEM（`-----BEGIN PRIVATE KEY-----`）

```typescript
import { decodePkcs8Pem, PK8_VERSION_V2 } from "pkcs8";

export fn inspectPkcs8Pem(pem: string): i64 {
    let mut res = decodePkcs8Pem(pem);
    if (!res.ok) {
        return res.err;
    }
    defer res.info.zeroize();

    if (res.info.version == PK8_VERSION_V2 && res.info.has_public_key) {
        return res.info.public_key_len as i64;
    }
    return res.info.private_key_len as i64;
}
```

## 核心 API 速览

- **`PrivateKeyInfo`**：
  - 字段：`version: u8`、`algorithm: AlgorithmIdentifier`、`private_key: [160]u8`、`private_key_len: usize`、`has_public_key: bool`、`public_key: [64]u8`、`public_key_len: usize`。
  - 方法：`empty()`、`ed25519(seed32)`、`ed25519V2(seed32, pub32)`、`p256(ec_priv_der)`、`zeroize()`。
- **DER/PEM 编解码**：`encodePkcs8Der`、`decodePkcs8Der`、`encodePkcs8Pem`、`decodePkcs8Pem`。
- **常量**：`PK8_VERSION_V1 (0)`、`PK8_VERSION_V2 (1)`、`PK8_MAX_PRIVKEY_LEN (160)`、`PK8_MAX_PUBKEY_LEN (64)`、`PK8_PEM_LABEL ("PRIVATE KEY")`，错误码 `PK8_OK (0)` 及 `-1..-12`。

## 许可证

MIT
