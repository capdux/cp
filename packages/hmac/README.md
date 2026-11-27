[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# hmac

Zero-heap RFC 2104 streaming **HMAC-SHA256** Message Authentication Code with **constant-time tag verification** and lowercase hex output for CP (RustCrypto `hmac` parity).

All key padding (`ipad`/`opad`), inner/outer SHA-256 states, and verification buffers live strictly on the stack with zero heap allocations.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **crates.io** |
| Original Crate | `hmac` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/RustCrypto/MACs` |
| License | MIT (upstream: MIT OR Apache-2.0) |

Keep the exact crate name when migrating from Rust to CP:

```rust
// Rust (crates.io: hmac)
use hmac::{Hmac, Mac};
let mut mac = Hmac::<Sha256>::new_from_slice(key)?;
mac.update(msg);
mac.verify_slice(&expected_tag)?;
```

```cp
// CP (cap: hmac) — zero-heap streaming HMAC-SHA256 + constant-time verify
import { HmacSha256, hmacSha256Hex } from "hmac";
let mac = HmacSha256.newFromSlice(key);
mac.update(msg);
const ok = mac.verifySlice(expected_tag);
```

## Installation

```bash
cap add hmac
```

## Usage

### Basic Usage: Streaming HMAC-SHA256 & One-Shot Hex Digest

```cp
import { HmacSha256, encodeHex32, hmacSha256Hex } from "hmac";

// 1. One-shot lowercase 64-byte hex MAC (RFC 4231 Test Case 2)
let hexBuf: [64]u8 = undefined;
const hexLen = hmacSha256Hex("Jefe", "what do ya want for nothing?", hexBuf[0..64]);
// hexLen == 64 -> "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843"

// 2. Streaming multi-chunk MAC into raw 32-byte digest
let mac = HmacSha256.newFromKey("webhook-secret");
mac.updateStr("timestamp=1700000000.");
mac.updateStr("{\"event\":\"paid\"}");
let tag: [32]u8 = undefined;
mac.finalize(tag[0..32]);

// Optional: convert raw 32-byte tag to 64-char lowercase hex
const n = encodeHex32(tag[0..32], hexBuf[0..64]); // 64
```

### Security & Edge Cases: Constant-Time Verification & Long Keys (> 64 Bytes)

```cp
import { HmacSha256 } from "hmac";

// 1. Constant-time verification (prevents timing side-channel attacks on MAC comparison)
let verifier = HmacSha256.newFromKey("webhook-secret");
verifier.updateStr("timestamp=1700000000.{\"event\":\"paid\"}");
const valid = verifier.verifySlice(expectedTag[0..32]); // uses ctEqual internally

// 2. Keys > 64 bytes are automatically pre-hashed with SHA-256 per RFC 2104
// Use `newFromSlice` for arbitrary binary keys or keys longer than 256 bytes!
let longKey: [131]u8 = [0xAA; 131];
let macLong = HmacSha256.newFromSlice(longKey[0..131]);
macLong.updateStr("Test Using Larger Than Block-Size Key - Hash Key First");
let digest: [32]u8 = undefined;
macLong.finalize(digest[0..32]);
```

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `HmacSha256.newFromSlice` | `static fn(key: []u8): HmacSha256` | Initializes HMAC-SHA256 from arbitrary-length binary key (pre-hashes if `> 64` bytes). |
| `HmacSha256.newFromKey` | `static fn(key: string): HmacSha256` | Convenience constructor from string key (up to 256 bytes). |
| `HmacSha256.update` | `fn(*_, data: []u8)` | Feeds a byte slice into the running MAC. |
| `HmacSha256.updateStr` | `fn(*_, msg: string)` | Feeds a `string` into the running MAC in 64-byte stack chunks. |
| `HmacSha256.finalize` | `fn(*_, out32: []u8)` | Finalizes and writes the 32-byte HMAC-SHA256 tag into `out32`. |
| `HmacSha256.verifySlice` | `fn(*_, expected: []u8): bool` | Finalizes and compares against `expected` in constant time (`ctEqual`). |
| `encodeHex32` | `fn(digest: []u8, out64: []u8): usize` | Encodes 32-byte digest as 64-byte lowercase hex; returns `64` or `0`. |
| `hmacSha256Hex` | `fn(key: string, msg: string, out64: []u8): usize` | One-shot string HMAC-SHA256 writing 64 lowercase hex bytes into `out64`. |

## Security Properties

- **Zero heap allocation**: Keys, pads, and digests stay on the stack.
- **Constant-time verification**: `verifySlice` checks `expected.len == 32` and compares via `ctEqual`.

## License

MIT — Clean-room CP implementation with RustCrypto `hmac` parity.
