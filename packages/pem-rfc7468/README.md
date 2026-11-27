[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# pem-rfc7468

> **Source Attribution**: Clean-room MIT CP port of **crates.io** [`pem-rfc7468`](https://github.com/RustCrypto/formats) (`v0.7` / `MIT OR Apache-2.0`), CP package `pem-rfc7468@1.0.0`.

Zero-allocation RFC 7468 PEM encoder and strict decoder in pure CP. Reuses `base64ct` for constant-time RFC 4648 Base64 encoding and canonical decoding.

## Why CP `pem-rfc7468`?

- **Zero Heap Allocation**: All framing, label validation, and Base64 decoding operate directly on caller-provided buffers.
- **Zero-Overhead Stack Initialization**: Internal fixed buffers (`PemLabel`, 48-byte encode chunks, 64-byte Base64 line buffers) use `undefined` initialization to eliminate redundant stack zeroing on hot paths.
- **Constant-Time Base64**: Delegates Base64 encoding/decoding to `base64ct` to mitigate timing side-channels when handling private keys.
- **Strict RFC 7468 Validation**: Verifies BEGIN/END label matching, RFC 7468 §2 label characters, canonical Base64 padding, and skips leading explanatory text.

## Zero-Cost Migration (Rust → CP)

Keep the exact same package name (`pem-rfc7468`) — no renaming required.

**Rust (`Cargo.toml` / `main.rs`):**
```rust
use pem_rfc7468::{encode, decode, LineEnding};
```

**CP (`cap.json` / `main.cp`):**
```typescript
import {
    PEM_OK, pemEncodedLen, encodePem, decodePem, decodeLabel, locatePem, isValidLabel
} from "pem-rfc7468";
```

## Quick Start

### Encode DER to PEM (64-Column Wrapped)

```typescript
import { pemEncodedLen, encodePem, PEM_OK } from "pem-rfc7468";

export fn demoEncode(der: &[]u8): i64 {
    let need = pemEncodedLen(11, der.len, false); // "CERTIFICATE".len == 11, LF endings
    let mut out: [1024]u8 = undefined;
    if (need > 1024) {
        return -1;
    }
    let written = encodePem("CERTIFICATE", der, false, out[0..need]);
    if (written < PEM_OK) {
        return written;
    }
    return written;
}
```

### Strict PEM Decoding & Zero-Copy Block Locating

```typescript
import { decodePem, decodeLabel, locatePem, isValidLabel } from "pem-rfc7468";

export fn demoDecode(pem_text: string): i64 {
    // 1. Zero-copy locate boundaries (skips leading comments/text)
    let slice = locatePem(pem_text);
    if (!slice.isOk()) {
        return slice.err;
    }

    // 2. Peek label without decoding Base64 body
    let mut label_buf: [64]u8 = undefined;
    let label_len = decodeLabel(pem_text, label_buf[0..64]);
    if (label_len < 0) {
        return label_len;
    }

    // 3. Full strict decode into caller buffer
    let mut out_label_len: usize = 0;
    let mut der_buf: [2048]u8 = undefined;
    let der_len = decodePem(pem_text, label_buf[0..64], &mut out_label_len, der_buf[0..2048]);
    return der_len;
}
```

## API Reference

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `isValidLabel` | `(label: string): bool` | Validates PEM label bytes per RFC 7468 §2 |
| `pemEncodedLen` | `(label_len: usize, der_len: usize, crlf: bool): usize` | Computes exact output byte length before encoding |
| `encodePem` | `(label: string, der: &[]u8, line_ending_crlf: bool, out: []u8): i64` | Encodes DER bytes to 64-col PEM; returns bytes written or `< 0` error |
| `locatePem` | `(input: string): PemSlice` | Zero-copy PEM block locator (`isOk()`, `label_start`, `body_start`, `end_pos`, `err`) |
| `decodeLabel` | `(input: string, label_out: []u8): i64` | Extracts PEM header label without decoding Base64 payload |
| `decodePem` | `(input: string, label_out: []u8, label_len_out: &mut usize, der_out: []u8): i64` | Strictly validates BEGIN/END labels and decodes Base64 to DER |

### Error Codes

| Constant | Value | Meaning |
| :--- | :---: | :--- |
| `PEM_OK` | `0` | Success threshold (`>= 0` indicates byte count) |
| `PEM_ERR_INVALID_LABEL` | `-1` | Invalid RFC 7468 label character or empty label |
| `PEM_ERR_BUFFER_TOO_SMALL` | `-2` | Caller output buffer too small |
| `PEM_ERR_MISSING_BEGIN` | `-3` | `-----BEGIN ` boundary not found |
| `PEM_ERR_MISSING_END` | `-4` | `-----END ` boundary not found |
| `PEM_ERR_LABEL_MISMATCH` | `-5` | BEGIN and END labels do not match |
| `PEM_ERR_INVALID_BASE64` | `-6` | Malformed or non-canonical Base64 payload |
| `PEM_ERR_INVALID_HEADER` | `-7` | Malformed header encapsulation line |
| `PEM_ERR_INVALID_FOOTER` | `-8` | Malformed footer encapsulation line |

## License

MIT
