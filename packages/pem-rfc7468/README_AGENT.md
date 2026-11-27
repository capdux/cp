# pem-rfc7468 (AI Agent Quick Ref)

- **Import**: `import { PEM_OK, PEM_LINE_WIDTH, pemEncodedLen, encodePem, decodePem, decodeLabel, locatePem, isValidLabel, PemLabel, PemSlice } from "pem-rfc7468";`
- **Source**: `crates.io:pem-rfc7468` (MIT, zero-alloc, constant-time Base64 via `base64ct`, `undefined` stack buffers).

## Core Signatures

```cp
export class PemLabel {
    buf: [64]u8, len: usize,
    fn empty(): PemLabel;
    fn isEmpty(): bool;
}
export class PemSlice {
    label_start: usize, label_len: usize,
    body_start: usize, body_len: usize,
    end_pos: usize, err: i64,
    fn isOk(): bool;
}

export fn isValidLabel(label: string): bool;
export fn pemEncodedLen(label_len: usize, der_len: usize, crlf: bool): usize;
export fn encodePem(label: string, der: &[]u8, line_ending_crlf: bool, out: []u8): i64;
export fn locatePem(input: string): PemSlice;
export fn decodeLabel(input: string, label_out: []u8): i64;
export fn decodePem(input: string, label_out: []u8, label_len_out: &mut usize, der_out: []u8): i64;
```

## Constants

- `PEM_LINE_WIDTH: usize = 64`
- `PEM_OK = 0`
- `PEM_ERR_INVALID_LABEL = -1`, `PEM_ERR_BUFFER_TOO_SMALL = -2`
- `PEM_ERR_MISSING_BEGIN = -3`, `PEM_ERR_MISSING_END = -4`, `PEM_ERR_LABEL_MISMATCH = -5`
- `PEM_ERR_INVALID_BASE64 = -6`, `PEM_ERR_INVALID_HEADER = -7`, `PEM_ERR_INVALID_FOOTER = -8`

## Minimal Pattern

```cp
let mut label_buf: [64]u8 = undefined;
let mut label_len: usize = 0;
let mut der_buf: [2048]u8 = undefined;
let n = decodePem(pem, label_buf[0..64], &mut label_len, der_buf[0..2048]);
if (n >= 0) {
    let der = der_buf[0..(n as usize)];
}
```

## Pitfalls

1. **Always check `< 0` before casting `i64` to `usize`**: `encodePem`, `decodeLabel`, and `decodePem` return byte count (`>= 0`) on success or negative `PEM_ERR_*` on failure.
2. **Pre-size encode output**: Call `pemEncodedLen(label.len, der.len, crlf)` first; undersized `out` returns `PEM_ERR_BUFFER_TOO_SMALL (-2)`.
3. **Pass `&mut usize` for `label_len_out`**: `decodePem` writes the decoded label length via `&mut label_len`.
4. **Multi-block PEM**: `decodePem` decodes only the first block; use `locatePem(input).end_pos` to advance or use `rustls-pemfile` for streaming bundles.
5. **No `pemEncodedLen` name collision**: Function is prefixed `pemEncodedLen` (not `encodedLen`).
