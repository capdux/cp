# @cap/sha1-crate

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:sha1` ([https://github.com/RustCrypto/hashes](https://github.com/RustCrypto/hashes))（原库名 `sha1` → CP 包名 `@cap/sha1-crate`）

Zero-allocation FIPS 180-4 SHA-1 cryptographic hash streaming implementation for CP — clean-room MIT parity with Rust `sha1` v0.10 (crates.io #35).

纯 CP 零堆分配 FIPS 180-4 SHA-1 密码学杂凑流式实现，支持分块流式更新、重置复用、原始 20 字节输出与 40 字节十六进制输出。

## Install / 安装

```bash
cap add @cap/sha1-crate
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `SHA1_DIGEST_LEN` | `usize = 20` | SHA-1 digest length in bytes |
| `SHA1_BLOCK_LEN` | `usize = 64` | SHA-1 compression block size in bytes |
| `Sha1Hasher.new` | `() -> Sha1Hasher` | Create a new streaming SHA-1 hasher |
| `Sha1Hasher.update` | `(*_, data: []u8)` | Feed raw bytes into the hasher |
| `Sha1Hasher.updateStr` | `(*_, s: string)` | Feed string bytes into the hasher |
| `Sha1Hasher.finalize` | `(*_, out20: []u8)` | Finalize and write 20-byte digest |
| `Sha1Hasher.finalizeHex` | `(*_, out40: []u8) -> usize` | Finalize and write 40-byte lowercase hex digest |
| `Sha1Hasher.reset` | `(*_)` | Reset hasher to initial FIPS 180-4 state |
| `sha1Bytes` | `(data: []u8, out20: []u8)` | One-shot SHA-1 for byte slices |
| `sha1Hex` | `(s: string, out40: []u8) -> usize` | One-shot lowercase hex SHA-1 for strings |

## Usage / 使用示例

```cp
import { Sha1Hasher, sha1Hex } from "@cap/sha1-crate";

let hex: [40]u8 = undefined;
const n = sha1Hex("abc", hex[0..40]); // "a9993e364706816aba3e25717850c26c9cd0d89d"

let h = Sha1Hasher.new();
h.updateStr("hello ");
h.updateStr("world");
* = h.finalizeHex(hex[0..40]);
```
