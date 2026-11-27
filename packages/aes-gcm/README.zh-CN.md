[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# aes-gcm

纯 CP 实现的零堆分配 **NIST SP 800-38D AES-128-GCM** 与 **AES-256-GCM** AEAD 认证加密库，底层组合 `aes` 与 `ghash-gf128`，内置恒定时间 16 字节标签校验。

## 源包溯源与零成本迁移

| 字段 | 说明 |
| :--- | :--- |
| **源平台** | [crates.io](https://crates.io/crates/aes-gcm) |
| **原包名与版本** | `aes-gcm`（RustCrypto AEADs，v0.10 对齐） |
| **上游仓库** | https://github.com/RustCrypto/AEADs |
| **CP 包名与版本** | `aes-gcm`（`1.0.0`） |
| **开源协议** | Apache-2.0 OR MIT |

在 CP 中保持原包名直接导入，无需改名：

```rust
// Rust (crates.io)
use aes_gcm::{Aes128Gcm, Aes256Gcm, KeyInit, aead::Aead};
```

```typescript
// CP (cap)
import { Aes128Gcm, Aes256Gcm } from "aes-gcm";
```

## 安装

```bash
cap add aes-gcm
```

## 核心特性与安全亮点

- **严格遵循 NIST SP 800-38D**：标准 96-bit（12 字节）Nonce 的 GCTR 加密与 GHASH 认证，完整支持附加认证数据（AAD）。
- **零堆分配**：`Aes128Gcm` 与 `Aes256Gcm` 在栈上直接缓存轮密钥与 GHASH 子密钥 `H`，无任何动态内存分配。
- **恒定时间校验**：`decrypt` 先通过 `agcmCtEq16` 以恒定时间校验 16 字节标签，不匹配立即返回 `false`。
- **实例可复用**：通过 `Aes128Gcm.new(key)` / `Aes256Gcm.new(key)` 一次初始化后，可复用同一实例处理多条不同 Nonce 的消息。

## 快速上手

### AES-128-GCM 加解密

```typescript
import { Aes128Gcm } from "aes-gcm";

fn main(): void {
    const key: [16]u8 = [
        0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
        0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08,
    ];
    const nonce: [12]u8 = [
        0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad,
        0xde, 0xca, 0xf8, 0x88,
    ];
    const aad: &[]u8 = "session-aad";
    const plaintext: &[]u8 = "Hello AES-128-GCM!";

    const gcm = Aes128Gcm.new(key);

    let ct: [18]u8 = undefined;
    const tag = gcm.encrypt(nonce, aad, plaintext, ct[0..]);

    let pt: [18]u8 = undefined;
    if (!gcm.decrypt(nonce, aad, ct[0..], tag, pt[0..])) {
        // 标签校验失败：拒绝密文
        return;
    }
}
```

### AES-256-GCM 加密封装

```typescript
import { Aes256Gcm } from "aes-gcm";

fn encryptMessage256(
    key: [32]u8,
    nonce: [12]u8,
    aad: &[]u8,
    plaintext: &[]u8,
    out_ct: []u8,
): [16]u8 {
    const gcm = Aes256Gcm.new(key);
    return gcm.encrypt(nonce, aad, plaintext, out_ct);
}
```

## API 速览

| 导出符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `Aes128Gcm.new` | `(key: [16]u8) -> Aes128Gcm` | 初始化 AES-128-GCM 并预计算 GHASH 子密钥 |
| `Aes128Gcm.encrypt` | `(nonce: [12]u8, aad: &[]u8, plaintext: &[]u8, out_ct: []u8) -> [16]u8` | 加密并返回 16 字节认证标签 |
| `Aes128Gcm.decrypt` | `(nonce: [12]u8, aad: &[]u8, ciphertext: &[]u8, tag: [16]u8, out_pt: []u8) -> bool` | 恒定时间校验标签并解密 |
| `Aes256Gcm.new` | `(key: [32]u8) -> Aes256Gcm` | 初始化 AES-256-GCM 并预计算 GHASH 子密钥 |
| `Aes256Gcm.encrypt` | `(nonce: [12]u8, aad: &[]u8, plaintext: &[]u8, out_ct: []u8) -> [16]u8` | 加密并返回 16 字节认证标签 |
| `Aes256Gcm.decrypt` | `(nonce: [12]u8, aad: &[]u8, ciphertext: &[]u8, tag: [16]u8, out_pt: []u8) -> bool` | 恒定时间校验标签并解密 |
| `agcmCtEq16` | `(a: [16]u8, b: [16]u8) -> bool` | 恒定时间 16 字节标签比较 |
| `agcmComputeTag` | `(h: [16]u8, e_j0: [16]u8, aad: &[]u8, ct: &[]u8) -> [16]u8` | 计算 16 字节 GHASH 标签 |

## 开源协议

Apache-2.0 OR MIT
