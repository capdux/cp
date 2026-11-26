# @cap/blake2-crate

Zero-allocation RFC 7693 BLAKE2s-256 streaming cryptographic hash & keyed MAC implementation for CP — clean-room MIT parity with Rust `blake2` v0.10 (crates.io #140).

纯 CP 零堆分配 RFC 7693 BLAKE2s-256 流式密码学杂凑与密钥 MAC 实现（支持 0..32 字节可选密钥），无堆分配、跨块延迟压缩。

## Install / 安装

```bash
cap add @cap/blake2-crate
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `BLAKE2S_DIGEST_LEN` | `usize = 32` | BLAKE2s-256 digest length in bytes |
| `BLAKE2S_BLOCK_LEN` | `usize = 64` | BLAKE2s compression block size in bytes |
| `Blake2s256.new` | `() -> Blake2s256` | Create unkeyed BLAKE2s-256 hasher |
| `Blake2s256.newKeyed` | `(key: []u8) -> Blake2s256?` | Create keyed BLAKE2s-256 MAC hasher (`0..=32` byte key) |
| `Blake2s256.update` | `(*_, data: []u8)` | Feed raw bytes into the hasher |
| `Blake2s256.updateStr` | `(*_, s: string)` | Feed string bytes into the hasher |
| `Blake2s256.finalize` | `(*_, out32: []u8)` | Finalize and write 32-byte digest |
| `Blake2s256.finalizeHex` | `(*_, out64: []u8) -> usize` | Finalize and write 64-byte lowercase hex digest |
| `blake2s256Bytes` | `(data: []u8, out32: []u8)` | One-shot BLAKE2s-256 for byte slices |
| `blake2s256Hex` | `(s: string, out64: []u8) -> usize` | One-shot lowercase hex BLAKE2s-256 for strings |

## Usage / 使用示例

```cp
import { Blake2s256, blake2s256Hex } from "@cap/blake2-crate";

let hex: [64]u8 = undefined;
* = blake2s256Hex("abc", hex[0..64]);

let mac = Blake2s256.newKeyed(key[0..16]).?;
mac.updateStr("payload");
* = mac.finalizeHex(hex[0..64]);
```
