<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`pem-rfc7468`](https://github.com/RustCrypto/formats)（`MIT OR Apache-2.0`）（包名 `@cap/pem-rfc7468`）。

# `@cap/pem-rfc7468`

Zero-allocation RFC 7468 PEM encoder and strict decoder in pure CP (RustCrypto `pem-rfc7468` parity). Reuses `@cap/base64ct` for constant-time RFC 4648 Base64 encoding and canonical decoding.

## API

- `isValidLabel(label: string): bool` — Validates PEM type labels per RFC 7468 §2
- `pemEncodedLen(label_len: usize, der_len: usize, crlf: bool): usize` — Exact PEM output buffer length pre-calculation (prefixed to avoid Zig flattening collision with `@cap/base64ct::encodedLen`)
- `encodePem(label: string, der: []const u8, line_ending_crlf: bool, out: []u8): i64` — Encodes DER bytes into 64-column wrapped PEM (`-----BEGIN <LABEL>-----` / `-----END <LABEL>-----`)
- `decodePem(input: string, label_out: []u8, label_len_out: *usize, der_out: []u8): i64` — Skips leading explanatory text, verifies matching BEGIN/END labels, and decodes Base64 payload via `@cap/base64ct`
- `decodeLabel(input: string, label_out: []u8): i64` — Extracts the PEM header label without decoding the body
- `locatePem(input: string): PemSlice` — Zero-copy PEM block locator

## Error Codes

- `PEM_ERR_INVALID_LABEL` (`-1`)
- `PEM_ERR_BUFFER_TOO_SMALL` (`-2`)
- `PEM_ERR_MISSING_BEGIN` (`-3`)
- `PEM_ERR_MISSING_END` (`-4`)
- `PEM_ERR_LABEL_MISMATCH` (`-5`)
- `PEM_ERR_INVALID_BASE64` (`-6`)
- `PEM_ERR_INVALID_HEADER` (`-7`)
- `PEM_ERR_INVALID_FOOTER` (`-8`)
