[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# base64url

Zero-heap RFC 4648 Section 5 Base64URL (unpadded & padded) encoder, decoder, validator, and standard Base64 converter in pure CP.

- **Package**: `base64url` (`v1.0.0`)
- **Upstream Alignment**: Clean-room CP port of `npm/base64url` (`MIT`), composing `data-encoding` for core Base64 primitives and `DecodeResult`.

## Installation & Import

```bash
cap add base64url
```

```typescript
import {
    encode, encodeBase64Url, encodePadded,
    decode, decodeBase64Url,
    encodedLen, encodedBase64UrlLen, encodedPaddedLen, decodedLen,
    fromBase64, fromStandardBase64, toBase64, toStandardBase64,
    padString, unpadString,
    validate, isValidBase64Url, validatePadded,
} from "base64url";
```

## Core API

### Encoding & Decoding

| Function | Signature | Description |
| --- | --- | --- |
| `encode` / `encodeBase64Url` | `(src: &[]u8, out: []u8): usize` | Encodes raw bytes to unpadded Base64URL (`-` and `_`), returning bytes written. |
| `encodePadded` | `(src: &[]u8, out: []u8): usize` | Encodes raw bytes to `=`-padded Base64URL, returning bytes written. |
| `decode` / `decodeBase64Url` | `(src: &[]u8, out: []u8): DecodeResult` | Decodes padded or unpadded Base64URL into `out`; check `.ok`, `.len`, `.error_pos`. |

### Length Computation

| Function | Signature | Description |
| --- | --- | --- |
| `encodedLen` / `encodedBase64UrlLen` | `(input_len: usize): usize` | Exact unpadded Base64URL output length for `input_len` raw bytes. |
| `encodedPaddedLen` | `(input_len: usize): usize` | Exact `=`-padded Base64URL output length (`((n + 2) / 3) * 4`). |
| `decodedLen` | `(src: string): usize?` | Exact decoded byte length, or `null` if `src` has invalid length (`len % 4 == 1`). |

### Standard Base64 Conversion, Padding & Validation

| Function | Signature | Description |
| --- | --- | --- |
| `fromBase64` / `fromStandardBase64` | `(b64: string, out: []u8): usize` | Converts standard Base64 (`+`, `/`, `=`) to unpadded Base64URL (`-`, `_`). |
| `toBase64` / `toStandardBase64` | `(b64url: string, out: []u8): usize` | Converts Base64URL to `=`-padded standard Base64 (`+`, `/`). |
| `padString` | `(s: string, out: []u8): usize` | Appends `=` padding to make length a multiple of 4. |
| `unpadString` | `(s: string): string` | Zero-copy strips trailing `=` padding characters. |
| `validate` / `isValidBase64Url` | `(s: string): bool` | Validates unpadded Base64URL alphabet and length. |
| `validatePadded` | `(s: string): bool` | Validates `=`-padded Base64URL alphabet, length (`% 4 == 0`), and padding position. |

## Examples

### 1. Encode & Decode JWT Payload

```typescript
import { encode, decode, encodedLen } from "base64url";

fn main() {
    let raw = "{\"sub\":\"42\"}";
    let enc: [64]u8 = undefined;
    let enc_len = encode(raw, enc[0..]);

    let dec: [64]u8 = undefined;
    let res = decode(enc[0..enc_len], dec[0..]);
    if (res.ok) {
        // dec[0..res.len] == raw
    }
}
```

### 2. Convert Between Standard Base64 and Base64URL

```typescript
import { fromBase64, toBase64, unpadString } from "base64url";

fn main() {
    let url_buf: [32]u8 = undefined;
    let n1 = fromBase64("ab+c/d==", url_buf[0..]); // "ab-c_d"

    let std_buf: [32]u8 = undefined;
    let n2 = toBase64("ab-c_d", std_buf[0..]);     // "ab+c/d=="
    let trimmed = unpadString("ab-c_d==");         // "ab-c_d"
}
```

### 3. Pre-Validate Before Decoding

```typescript
import { validate, decodedLen } from "base64url";

fn main() {
    let token = "SGVsbG8td29ybGQ";
    if (validate(token)) {
        if (let out_len = decodedLen(token)) {
            // out_len == 11
        }
    }
}
```

## Differences from Upstream & Highlights

- **Zero Heap Allocation**: All encoders and converters write directly into caller-provided `out: []u8` buffers instead of allocating JS `String`/`Buffer` objects.
- **Structured Decode Errors**: `decode` / `decodeBase64Url` return `DecodeResult` (`ok`, `len`, `error_pos`) instead of silently skipping invalid characters.
- **Zero-Copy `unpadString`**: Returns a sub-slice of the input string without copying.
