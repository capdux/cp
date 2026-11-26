# @cap/cookie-signature

Zero-heap-allocation HMAC-SHA256 signed cookie generator and constant-time verifier in pure CP (parity with npm `cookie-signature`), directly reusing `@cap/base64ct` for constant-time unpadded Base64 encoding.

## Dependencies

- `@cap/base64ct` (`^1.0.0`)

## Features

- `sha256(data, out32)` & `hmacSha256(key, msg, out32)` (RFC 2104, overflow-safe Debug arithmetic)
- `ctEqual(a, b): bool` (constant-time byte comparison)
- `sign(val, secret, out): usize` (`val + "." + base64_unpadded(HMAC-SHA256(secret, val))`)
- `unsign(signed_cookie, secret): UnsignResult { ok, val_len }`

## License

MIT
