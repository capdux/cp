[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/sodium

libsodium 1.0.20 modern cryptography suite (Ed25519, XChaCha20-Poly1305 AEAD, BLAKE2b, CSPRNG) for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/sodium` |
| 版本 | `1.0.20` |
| 许可证 | `ISC` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/sodium
```

```cp
import {
    SodiumError,
    Ed25519Keypair,
    Ed25519Signature,
    Sodium,
    sodium_init,
    sodium_version_string,
    randombytes_buf,
    crypto_sign_ed25519_keypair,
} from "@cap/sodium";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `SodiumError` | `InitFailed, InvalidKeyLength, InvalidNonceLength, InvalidSignatureLength, InvalidHashLength, BufferTooSmall, CiphertextTooShort, KeypairFailed ...` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Ed25519Keypair` | `class` | 字段: `pk: [32]u8, sk: [64]u8` |
| `Ed25519Signature` | `class` | 字段: `bytes: [64]u8` |
| `Sodium` | `class` | 方法: `init, versionString, randombytesBuf, ed25519Keypair, ed25519SignDetached, ed25519VerifyDetached, +3 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `sodium_init` | `(): i32` | 调用 `sodium_init` 并返回 `i32`。 |
| `sodium_version_string` | `(): [*:0]&u8!?` | 调用 `sodium_version_string` 并返回 `[*:0]&u8!?`。 |
| `randombytes_buf` | `(buf: [*]u8!, size: usize)` | 就地执行 `randombytes_buf` 操作。 |
| `crypto_sign_ed25519_keypair` | `(pk: [*]u8!, sk: [*]u8!): i32` | 调用 `crypto_sign_ed25519_keypair` 并返回 `i32`。 |
| `crypto_sign_ed25519_detached` | `(sig: [*]u8!, siglen_p: *u64!?, m: [*]&u8!?, mlen: u64, sk: [*]&u8!): i32` | 调用 `crypto_sign_ed25519_detached` 并返回 `i32`。 |
| `crypto_sign_ed25519_verify_detached` | `(sig: [*]&u8!, m: [*]&u8!?, mlen: u64, pk: [*]&u8!): i32` | 调用 `crypto_sign_ed25519_verify_detached` 并返回 `i32`。 |
| `crypto_aead_xchacha20poly1305_ietf_encrypt` | `(c: [*]u8!, clen_p: *u64!?, m: [*]&u8!?, mlen: u64, ad: [*]&u8!?, adlen: u64, nsec: [*]&u8!?, npub: [*]&u8!, k: [*]&u8!): i32` | 调用 `crypto_aead_xchacha20poly1305_ietf_encrypt` 并返回 `i32`。 |
| `crypto_aead_xchacha20poly1305_ietf_decrypt` | `(m: [*]u8!?, mlen_p: *u64!?, nsec: [*]u8!?, c: [*]&u8!, clen: u64, ad: [*]&u8!?, adlen: u64, npub: [*]&u8!, k: [*]&u8!): i32` | 调用 `crypto_aead_xchacha20poly1305_ietf_decrypt` 并返回 `i32`。 |
| `crypto_generichash` | `(out: [*]u8!, outlen: usize, in_buf: [*]&u8!?, inlen: u64, key: [*]&u8!?, keylen: usize): i32` | 调用 `crypto_generichash` 并返回 `i32`。 |

## 最小调用示例

```cp
import { sodium_init } from "@cap/sodium";

pub fn main() {
    let result = sodium_init();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
