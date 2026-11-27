[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# hmac

Zero-heap RFC 2104 streaming HMAC-SHA256 and HMAC-SHA512 message authentication codes with constant-time verification and lowercase hex encoding (reusing `cookie-signature` SHA-256 primitives).

Aligned with Rust crates.io [`hmac`](https://github.com/RustCrypto/MACs) (MIT OR Apache-2.0).

## Installation & Import

```bash
cap add hmac
```

```cp
import {
    HMAC_SHA256_LEN,
    HMAC_BLOCK_SIZE,
    HMAC_SHA512_LEN,
    HMAC_SHA512_BLOCK_SIZE,
    HmacSha256,
    HmacSha512,
    hmacSha256Bytes,
    hmacSha256Hex,
    hmacSha512,
    hmacSha512Hex,
    ctEqualBytes,
    encodeHex32,
    encodeHex64,
} from "hmac";
```

## Core API

### Constants & Helpers

| Symbol | Type / Signature | Description |
| --- | --- | --- |
| `HMAC_SHA256_LEN` / `HMAC_BLOCK_SIZE` | `usize = 32` / `64` | SHA-256 MAC length and block size in bytes. |
| `HMAC_SHA512_LEN` / `HMAC_SHA512_BLOCK_SIZE` | `usize = 64` / `128` | SHA-512 MAC length and block size in bytes. |
| `ctEqualBytes` | `(a: []const u8, b: []const u8): bool` | Constant-time byte-slice comparison resistant to timing side channels. |
| `encodeHex32` | `(digest: []u8, out64: []u8): usize` | Encodes 32-byte digest into 64 lowercase hex bytes. |
| `encodeHex64` | `(digest: []const u8, out128: []u8): usize` | Encodes 64-byte digest into 128 lowercase hex bytes. |

### Streaming Classes

| Class | Methods | Description |
| --- | --- | --- |
| `HmacSha256` | `newFromSlice(key: []u8)`, `newFromConstSlice(key: []const u8)`, `newFromKey(key: string)`, `update(*self, data: []u8)`, `updateConst(*self, data: []const u8)`, `updateStr(*self, msg: string)`, `finalize(*self, out32: []u8)`, `verifySlice(*self, expected: []u8): bool`, `verifyConstSlice(*self, expected: []const u8): bool` | Streaming RFC 2104 HMAC-SHA256 state with constant-time `verify*`. |
| `HmacSha512` | `newFromSlice(key: []const u8)`, `newFromKey(key: string)`, `update(*self, data: []const u8)`, `updateStr(*self, msg: string)`, `finalize(*self, out64: []u8)`, `verifySlice(*self, expected: []const u8): bool` | Streaming RFC 2104 HMAC-SHA512 state with constant-time `verifySlice`. |

### One-Shot Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hmacSha256Bytes` | `(key: []const u8, msg: []const u8, out32: []u8)` | Computes raw 32-byte HMAC-SHA256 into `out32`. |
| `hmacSha256Hex` | `(key: string, msg: string, out64: []u8): usize` | Computes 64-byte lowercase hex HMAC-SHA256 into `out64`; returns `64`. |
| `hmacSha512` | `(key: []const u8, msg: []const u8, out64: []u8)` | Computes raw 64-byte HMAC-SHA512 into `out64`. |
| `hmacSha512Hex` | `(key: string, msg: string, out128: []u8): usize` | Computes 128-byte lowercase hex HMAC-SHA512 into `out128`; returns `128`. |

## Examples

### 1. One-Shot HMAC-SHA256 & HMAC-SHA512 Hex

```cp
import { print } from "cap:io";
import { hmacSha256Hex, hmacSha512Hex } from "hmac";

pub fn main() {
    let hex256: [64]u8 = undefined;
    let n256 = hmacSha256Hex("secret-key", "hello world", hex256[0..64]);
    print("HMAC-SHA256: {}\n", hex256[0..n256]);

    let hex512: [128]u8 = undefined;
    let n512 = hmacSha512Hex("secret-key", "hello world", hex512[0..128]);
    print("HMAC-SHA512: {}\n", hex512[0..n512]);
}
```

### 2. Streaming HMAC-SHA256 & Constant-Time Verification

```cp
import { print } from "cap:io";
import { HmacSha256 } from "hmac";

pub fn main() {
    let mac = HmacSha256.newFromKey("webhook-secret");
    mac.updateStr("chunk-1:");
    mac.updateStr("chunk-2");
    let tag: [32]u8 = undefined;
    mac.finalize(tag[0..32]);

    let verifier = HmacSha256.newFromKey("webhook-secret");
    verifier.updateStr("chunk-1:chunk-2");
    let ok = verifier.verifyConstSlice(tag[0..32]);
    print("verified={}\n", ok);
}
```

## Differences from Upstream & Zero-Heap Highlights

- **Zero heap allocation**: All hash states, key pads (`o_key_pad`), and output digests reside on caller stack buffers.
- **Timing-safe verification**: `verifySlice`, `verifyConstSlice`, and `ctEqualBytes` compare every byte in constant time to prevent timing side-channel leaks.
- **Concrete SHA-256 & SHA-512 types**: Instead of Rust generic `Hmac<D>` trait bounds, exposes monomorphic `HmacSha256` and `HmacSha512` classes plus one-shot hex helpers.
