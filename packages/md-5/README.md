[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/md5-crate

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:md-5` ([https://github.com/RustCrypto/hashes](https://github.com/RustCrypto/hashes))（原库名 `md-5` → CP 包名 `@cap/md5-crate`）

Zero-allocation RFC 1321 MD5 cryptographic hash streaming implementation for CP — clean-room MIT parity with Rust `md-5` / `md5` (crates.io #65).

纯 CP 零堆分配 RFC 1321 MD5 流式杂凑实现，支持分块流式更新、重置复用、原始 16 字节输出与 32 字节十六进制输出。

## Install / 安装

```bash
cap add @cap/md5-crate
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `MD5_DIGEST_LEN` | `usize = 16` | MD5 digest length in bytes |
| `MD5_BLOCK_LEN` | `usize = 64` | MD5 compression block size in bytes |
| `Md5Hasher.new` | `() -> Md5Hasher` | Create a new streaming MD5 hasher |
| `Md5Hasher.update` | `(*_, data: []u8)` | Feed raw bytes into the hasher |
| `Md5Hasher.updateStr` | `(*_, s: string)` | Feed string bytes into the hasher |
| `Md5Hasher.finalize` | `(*_, out16: []u8)` | Finalize and write 16-byte digest |
| `Md5Hasher.finalizeHex` | `(*_, out32: []u8) -> usize` | Finalize and write 32-byte lowercase hex digest |
| `Md5Hasher.reset` | `(*_)` | Reset hasher to initial RFC 1321 state |
| `md5Bytes` | `(data: []u8, out16: []u8)` | One-shot MD5 for byte slices |
| `md5Hex` | `(s: string, out32: []u8) -> usize` | One-shot lowercase hex MD5 for strings |

## Usage / 使用示例

```cp
import { Md5Hasher, md5Hex } from "@cap/md5-crate";

let hex: [32]u8 = undefined;
const n = md5Hex("abc", hex[0..32]); // "900150983cd24fb0d6963f7d28e17f72"

let h = Md5Hasher.new();
h.updateStr("message ");
h.updateStr("digest");
* = h.finalizeHex(hex[0..32]);
```
