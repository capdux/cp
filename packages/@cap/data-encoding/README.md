# @cap/data-encoding

Zero-allocation strict RFC 4648 Base16/Hex, Base32, and Base64/Base64URL codec with canonical trailing-bit validation and precise error position reporting for CP — clean-room MIT parity with Rust `data-encoding` v2 (800M+ downloads).

纯 CP 零堆分配严格 RFC 4648 编解码库，内置非零尾位拒绝（RFC 4648 §3.5）与精确错误偏移定位，适用于 JWT、TOTP/OTP 密钥、哈希指纹与二进制协议传输。

## Install / 安装

```bash
cap add @cap/data-encoding
```

## API

### Result Type

- `DecodeResult { ok: bool, len: usize, error_pos: usize }`

### Functions

| Symbol | Signature | Description |
| --- | --- | --- |
| `hexLowerEncode` | `(src: []const u8, out: []u8) -> usize` | Lowercase RFC 4648 Base16 encoding |
| `hexUpperEncode` | `(src: []const u8, out: []u8) -> usize` | Uppercase RFC 4648 Base16 encoding |
| `hexDecode` | `(src: []const u8, out: []u8) -> DecodeResult` | Case-insensitive Base16 decoding; rejects odd lengths & invalid chars |
| `base32Encode` | `(src: []const u8, pad: bool, out: []u8) -> usize` | RFC 4648 Base32 encoding (`A-Z`, `2-7`) |
| `base32Decode` | `(src: []const u8, out: []u8) -> DecodeResult` | Strict Base32 decoding (padded or unpadded, rejects non-zero trailing bits) |
| `base64Encode` | `(src: []const u8, url_safe: bool, pad: bool, out: []u8) -> usize` | RFC 4648 Base64 & Base64URL encoding |
| `base64Decode` | `(src: []const u8, url_safe: bool, out: []u8) -> DecodeResult` | Strict Base64 & Base64URL decoding (rejects non-zero trailing bits) |
| `base64EncodeLen` / `base32EncodeLen` | `(n: usize, pad: bool) -> usize` | Exact encoded buffer length calculation |

## Usage / 使用示例

```cp
import { base64Encode, base64Decode } from "@cap/data-encoding";

let enc: [16]u8 = undefined;
let dec_buf: [16]u8 = undefined;
let n = base64Encode("foobar", false, true, enc[0..16]); // "Zm9vYmFy" (8 bytes)
let res = base64Decode(enc[0..n], false, dec_buf[0..16]); // res.ok == true, res.len == 6
```
