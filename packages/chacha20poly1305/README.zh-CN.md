[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# chacha20poly1305

纯 CP 实现的零堆分配 **RFC 8439 ChaCha20-Poly1305** 与 **XChaCha20-Poly1305** AEAD 认证加密算法库，底层复用 `poly1305`、`rand_chacha` 与 `subtle`，具备恒定时间标签校验与临时密钥自动清零能力。

## 源包溯源与零成本迁移

| 字段 | 说明 |
| :--- | :--- |
| **源平台** | [crates.io](https://crates.io/crates/chacha20poly1305) |
| **原包名与版本** | `chacha20poly1305`（RustCrypto AEADs，v0.10 对齐） |
| **上游仓库** | https://github.com/RustCrypto/AEADs |
| **CP 包名与版本** | `chacha20poly1305`（`1.0.0`） |
| **开源协议** | MIT / Apache-2.0 |

在 CP 中保持原包名直接导入，无需改名：

```rust
// Rust (crates.io)
use chacha20poly1305::{ChaCha20Poly1305, XChaCha20Poly1305, aead::Aead};
```

```typescript
// CP (cap)
import {
    aeadEncrypt,
    aeadDecrypt,
    xchacha20Poly1305Encrypt,
    xchacha20Poly1305Decrypt,
    CHACHA20_KEY_LEN,
    CHACHA20_NONCE_LEN,
    CHACHA20_TAG_LEN,
    XCHACHA20_NONCE_LEN,
} from "chacha20poly1305";
```

## 安装

```bash
cap add chacha20poly1305
```

## 核心特性与安全亮点

- **零堆分配**：加解密、Poly1305 一次性密钥派生与 HChaCha20 子密钥派生全部在调用方提供的固定缓冲区上完成。
- **恒定时间标签校验**：`aeadDecrypt` 与 `xchacha20Poly1305Decrypt` 使用恒定时间比较校验 16 字节认证标签，抵御时序侧信道；校验失败时不泄露明文。
- **支持 12 字节与 24 字节 Nonce**：除标准 RFC 8439（12 字节 Nonce）外，完整提供 XChaCha20-Poly1305（24 字节 Nonce），适合大规模随机 Nonce 场景。
- **密钥材料自动清零**：一次性 Poly1305 密钥与 HChaCha20 子密钥在使用完毕后立即通过 `zeroizeBytes` 擦除。

## 快速上手

### ChaCha20-Poly1305 AEAD（12 字节 Nonce）

```typescript
import {
    aeadEncrypt,
    aeadDecrypt,
    CHACHA20_TAG_LEN,
} from "chacha20poly1305";

fn main(): void {
    const key: [32]u8 = [
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
        0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    ];
    const nonce: [12]u8 = [0x07, 0x00, 0x00, 0x00, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47];
    const aad: &[]u8 = "header-v1";
    const plaintext: &[]u8 = "secret agent payload";

    let ciphertext: [20]u8 = undefined;
    let tag: [CHACHA20_TAG_LEN]u8 = undefined;

    if (!aeadEncrypt(key[0..], nonce[0..], aad, plaintext, ciphertext[0..], tag[0..])) {
        return;
    }

    let decrypted: [20]u8 = undefined;
    if (!aeadDecrypt(key[0..], nonce[0..], aad, ciphertext[0..], tag[0..], decrypted[0..])) {
        // 认证失败：密文、标签或 AAD 被篡改
        return;
    }
}
```

### XChaCha20-Poly1305 AEAD（24 字节 Nonce）

```typescript
import {
    xchacha20Poly1305Encrypt,
    xchacha20Poly1305Decrypt,
} from "chacha20poly1305";

fn sealWithRandomNonce(
    key: &[]u8,
    nonce24: &[]u8,
    aad: &[]u8,
    plaintext: &[]u8,
    out_ct: []u8,
    out_tag: []u8,
): bool {
    return xchacha20Poly1305Encrypt(key, nonce24, aad, plaintext, out_ct, out_tag);
}
```

## API 速览

| 导出符号 | 签名 / 值 | 说明 |
| :--- | :--- | :--- |
| `CHACHA20_KEY_LEN` | `32` | 密钥长度（字节） |
| `CHACHA20_NONCE_LEN` | `12` | RFC 8439 Nonce 长度 |
| `CHACHA20_BLOCK_LEN` | `64` | ChaCha20 密钥流块大小 |
| `CHACHA20_TAG_LEN` | `16` | Poly1305 认证标签长度 |
| `XCHACHA20_NONCE_LEN` | `24` | XChaCha20 扩展 Nonce 长度 |
| `aeadEncrypt` | `(key, nonce, aad, plaintext, ciphertext, tag) -> bool` | RFC 8439 认证加密 |
| `aeadDecrypt` | `(key, nonce, aad, ciphertext, tag, plaintext) -> bool` | 恒定时间校验并解密 |
| `xchacha20Poly1305Encrypt` | `(key, nonce24, aad, plaintext, ciphertext, tag) -> bool` | XChaCha20-Poly1305 认证加密 |
| `xchacha20Poly1305Decrypt` | `(key, nonce24, aad, ciphertext, tag, plaintext) -> bool` | XChaCha20-Poly1305 校验并解密 |
| `xchacha20HChaCha20` | `(key, nonce16, out_subkey) -> bool` | 派生 32 字节 HChaCha20 子密钥 |
| `chacha20Block` | `(key, counter, nonce, out) -> bool` | 生成 64 字节 ChaCha20 密钥流块 |
| `chacha20Xor` | `(key, counter, nonce, input, out) -> bool` | 流加密 XOR |
| `poly1305KeyGen` | `(key, nonce, out) -> bool` | 生成 32 字节 Poly1305 一次性密钥 |

## 开源协议

MIT / Apache-2.0
