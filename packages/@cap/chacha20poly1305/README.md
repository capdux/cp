# @cap/chacha20poly1305

Zero-allocation RFC 8439 ChaCha20-Poly1305 AEAD authenticated encryption for CP — clean-room MIT parity with Rust `chacha20poly1305` v0.10 (crates.io #105).

纯 CP 零堆分配 RFC 8439 ChaCha20-Poly1305 认证加密（AEAD），组合复用 `@cap/chacha20-crate`、`@cap/poly1305` 与 `@cap/subtle-ct`，支持分离标签原地加解密、恒定时间标签验签与敏感密钥零化。

## Install / 安装

```bash
cap add @cap/chacha20poly1305
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `AEAD_KEY_LEN` | `usize = 32` | AEAD key size (256 bits) |
| `AEAD_NONCE_LEN` | `usize = 12` | AEAD nonce size (96 bits) |
| `AEAD_TAG_LEN` | `usize = 16` | Poly1305 authentication tag size (128 bits) |
| `ChaCha20Poly1305.new` | `(key: []u8) -> ChaCha20Poly1305?` | Construct AEAD cipher from 32-byte key |
| `ChaCha20Poly1305.encryptInPlaceDetached` | `(nonce, aad, buffer, tag_out16) -> bool` | Encrypt `buffer` in-place and write 16-byte tag |
| `ChaCha20Poly1305.decryptInPlaceDetached` | `(nonce, aad, buffer, tag16) -> bool` | Constant-time verify tag first; decrypt in-place only on success |

## Usage / 使用示例

```cp
import { ChaCha20Poly1305 } from "@cap/chacha20poly1305";

let aead = ChaCha20Poly1305.new(key[0..32]).?;
let tag: [16]u8 = undefined;
* = aead.encryptInPlaceDetached(nonce[0..12], aad[0..aad_len], buf[0..len], tag[0..16]);
const ok = aead.decryptInPlaceDetached(nonce[0..12], aad[0..aad_len], buf[0..len], tag[0..16]);
```
