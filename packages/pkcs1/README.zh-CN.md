[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# pkcs1

> **源包溯源**：吸收自 **crates.io** [`pkcs1`](https://github.com/RustCrypto/formats)（`MIT`），CP 官方洁净室实现 `pkcs1@1.0.0`。

纯 CP 实现的零堆分配 RFC 8017 附录 A.1.1（`RSAPublicKey`）与附录 A.1.2（`RSAPrivateKey`）DER/PEM 编解码器。底层复用 `der`、`pem-rfc7468` 与 `subtle`，提供严格 ASN.1 DER 解析、常量时间 Base64 PEM 封装与安全密钥擦除。

## 核心亮点

- **零堆分配（支持至 4096-bit RSA）**：模数 `n`（最大 512 字节）与素数因子（最大 256 字节）全部内联栈存储。
- **敏感私钥安全擦除**：`RsaPrivateKey.zeroize()` 调用 `subtle` 的 `zeroizeBytes` 彻底擦除 `n`、`d`、`p`、`q`、`dp`、`dq`、`qinv`，防止编译器优化掉清零操作。
- **零栈清零优化**：内部大尺寸栈数组（`[512]u8`、`[256]u8`、`[513]u8` 临时缓冲）采用 `undefined` 初始化，大幅降低调用开销。
- **严格标准校验**：拒绝负数整数、非规范 DER 长度、尾部残留字节及不匹配的 PEM 标签（`RSA PUBLIC KEY` / `RSA PRIVATE KEY`）。

## 零成本迁移（Rust → CP）

直接沿用原包名 `pkcs1`，无需改名：

**Rust：**
```rust
use pkcs1::{DecodeRsaPrivateKey, EncodeRsaPublicKey, RsaPrivateKey, RsaPublicKey};
```

**CP：**
```typescript
import {
    RsaPublicKey, RsaPrivateKey, modulusBitLen,
    encodeRsaPublicKeyDer, decodeRsaPublicKeyDer,
    encodeRsaPublicKeyPem, decodeRsaPublicKeyPem,
    encodeRsaPrivateKeyDer, decodeRsaPrivateKeyDer,
    encodeRsaPrivateKeyPem, decodeRsaPrivateKeyPem
} from "pkcs1";
```

## 实战示例

### 1. 解析 RSA 私钥 PEM、导出公钥并自动 `zeroize()` 擦除

```typescript
import {
    decodeRsaPrivateKeyPem, encodeRsaPublicKeyPem, modulusBitLen
} from "pkcs1";

export fn extractPublicPem(priv_pem: string, pub_pem_out: []u8): i64 {
    let mut res = decodeRsaPrivateKeyPem(priv_pem);
    if (!res.ok) {
        return res.err;
    }
    defer res.key.zeroize(); // 退出作用域时安全擦除私钥内存

    let pub_key = res.key.publicKey();
    let bits = modulusBitLen(pub_key);
    if (bits < 2048) {
        return -100; // 强制最低 2048 位 RSA 强度
    }
    return encodeRsaPublicKeyPem(pub_key, false, pub_pem_out);
}
```

### 2. 构造并编码 `RsaPublicKey` 为 DER

```typescript
import { RsaPublicKey, encodeRsaPublicKeyDer } from "pkcs1";

export fn buildPubDer(modulus: &[]u8, out: []u8): i64 {
    let res = RsaPublicKey.new(modulus, 65537);
    if (!res.ok) {
        return res.err;
    }
    return encodeRsaPublicKeyDer(res.key, out);
}
```

## 核心 API 速览

- **`RsaPublicKey`**：`n: [512]u8`、`n_len: usize`、`e: u64`；方法 `empty()`、`new(n_bytes, e): RsaPublicKeyResult`。
- **`RsaPrivateKey`**：含 `version`、`n`、`e`、`d`、`p`、`q`、`dp`、`dq`、`qinv` 及对应 `*_len`；方法 `empty()`、`publicKey()`、`zeroize()`。
- **工具函数**：`modulusBitLen(pub_key: RsaPublicKey): usize`。
- **DER/PEM 编解码**：`encode/decode` × `RsaPublicKey/RsaPrivateKey` × `Der/Pem` 共 8 个函数。
- **错误码**：`PK1_OK (0)`、`PK1_ERR_BUFFER_TOO_SMALL (-1)` ~ `PK1_ERR_INVALID_PEM_LABEL (-9)`。

## 许可证

MIT
