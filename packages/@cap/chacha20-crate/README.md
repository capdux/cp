# @cap/chacha20-crate

Zero-allocation RFC 8439 ChaCha20 stream cipher (256-bit key, 96-bit nonce, 32-bit block counter) for CP — clean-room MIT parity with Rust `chacha20` v0.9 (crates.io #115).

纯 CP 零堆分配 RFC 8439 ChaCha20 流密码实现，支持 256 位密钥、96 位 Nonce、32 位块计数器跳转 (`seek`) 与跨 64 字节块任意长度流式原地加解密 (`applyKeystream`)。

## Install / 安装

```bash
cap add @cap/chacha20-crate
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `CHACHA20_KEY_LEN` | `usize = 32` | ChaCha20 key size (256 bits) |
| `CHACHA20_NONCE_LEN` | `usize = 12` | ChaCha20 nonce size (96 bits) |
| `CHACHA20_BLOCK_LEN` | `usize = 64` | ChaCha20 keystream block size (512 bits) |
| `chacha20Block` | `(key: []u8, counter: u32, nonce: []u8, out64: []u8) -> bool` | Generate one 64-byte keystream block |
| `ChaCha20Cipher.new` | `(key: []u8, nonce: []u8) -> ChaCha20Cipher?` | Create cipher initialized at block counter `0` |
| `ChaCha20Cipher.newWithCounter` | `(key: []u8, nonce: []u8, counter: u32) -> ChaCha20Cipher?` | Create cipher at specified 32-bit block counter |
| `ChaCha20Cipher.seek` | `(*_, block_counter: u32)` | Seek cipher to a specific 32-bit block counter |
| `ChaCha20Cipher.applyKeystream` | `(*_, data: []u8)` | XOR keystream into `data` in-place (streaming) |

## Usage / 使用示例

```cp
import { ChaCha20Cipher } from "@cap/chacha20-crate";

let cipher = ChaCha20Cipher.newWithCounter(key[0..32], nonce[0..12], 1 as u32)!;
cipher.applyKeystream(buf[0..len]);
```
