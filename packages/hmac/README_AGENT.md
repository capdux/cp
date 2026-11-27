# hmac — AI Agent Quick Ref

Zero-heap RFC 2104 streaming HMAC-SHA256 / HMAC-SHA512 with constant-time verification and hex output (crates.io `hmac` parity).

## Import

```cp
import {
    HMAC_SHA256_LEN, HMAC_BLOCK_SIZE, HMAC_SHA512_LEN, HMAC_SHA512_BLOCK_SIZE,
    HmacSha256, HmacSha512,
    hmacSha256Bytes, hmacSha256Hex, hmacSha512, hmacSha512Hex,
    ctEqualBytes, encodeHex32, encodeHex64,
} from "hmac";
```

## API Signatures

```cp
const HMAC_SHA256_LEN: usize;        // 32
const HMAC_BLOCK_SIZE: usize;        // 64
const HMAC_SHA512_LEN: usize;        // 64
const HMAC_SHA512_BLOCK_SIZE: usize; // 128

class HmacSha256 {
    static fn newFromSlice(key: []u8): HmacSha256;
    static fn newFromConstSlice(key: []const u8): HmacSha256;
    static fn newFromKey(key: string): HmacSha256;
    fn update(*self, data: []u8);
    fn updateConst(*self, data: []const u8);
    fn updateStr(*self, msg: string);
    fn finalize(*self, out32: []u8);
    fn verifySlice(*self, expected: []u8): bool;
    fn verifyConstSlice(*self, expected: []const u8): bool;
}

class HmacSha512 {
    static fn newFromSlice(key: []const u8): HmacSha512;
    static fn newFromKey(key: string): HmacSha512;
    fn update(*self, data: []const u8);
    fn updateStr(*self, msg: string);
    fn finalize(*self, out64: []u8);
    fn verifySlice(*self, expected: []const u8): bool;
}

fn ctEqualBytes(a: []const u8, b: []const u8): bool;
fn encodeHex32(digest: []u8, out64: []u8): usize;
fn encodeHex64(digest: []const u8, out128: []u8): usize;
fn hmacSha256Bytes(key: []const u8, msg: []const u8, out32: []u8);
fn hmacSha256Hex(key: string, msg: string, out64: []u8): usize;
fn hmacSha512(key: []const u8, msg: []const u8, out64: []u8);
fn hmacSha512Hex(key: string, msg: string, out128: []u8): usize;
```

## Minimal Example

```cp
import { HmacSha256, hmacSha256Hex } from "hmac";

let hex: [64]u8 = undefined;
let _ = hmacSha256Hex("key", "msg", hex[0..64]);

let m = HmacSha256.newFromKey("key");
m.updateStr("msg");
let out: [32]u8 = undefined;
m.finalize(out[0..32]);
```

## Pitfalls & Borrowing Rules

- **Output buffer sizes**: `out32` needs >= 32 bytes, `out64` needs >= 64 bytes, `out128` needs >= 128 bytes.
- **Single-use finalization**: `finalize` and `verify*` consume/mutate internal hash state; construct a fresh `HmacSha256` / `HmacSha512` for each message.
- **Slice mutability**: For `HmacSha256`, use `newFromConstSlice` / `updateConst` / `verifyConstSlice` when passing `[]const u8`; `HmacSha512` methods accept `[]const u8` directly.
- **Timing safety**: Always prefer `verifySlice` / `verifyConstSlice` / `ctEqualBytes` over `==` when checking MAC tags.
