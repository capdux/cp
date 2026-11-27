# pkcs1 (AI Agent Quick Ref)

- **Import**: `import { RsaPublicKey, RsaPublicKeyResult, RsaPrivateKey, RsaPrivateKeyResult, modulusBitLen, encodeRsaPublicKeyDer, decodeRsaPublicKeyDer, encodeRsaPublicKeyPem, decodeRsaPublicKeyPem, encodeRsaPrivateKeyDer, decodeRsaPrivateKeyDer, encodeRsaPrivateKeyPem, decodeRsaPrivateKeyPem, PK1_OK, PK1_MAX_MODULUS_LEN, PK1_MAX_PRIME_LEN } from "pkcs1";`
- **Source**: `crates.io:pkcs1` (MIT, zero-alloc up to 4096-bit RSA, wraps `der` + `pem-rfc7468` + `subtle`, `undefined` stack buffers).

## Core Signatures

```cp
export class RsaPublicKey {
    n: [512]u8, n_len: usize, e: u64,
    fn empty(): RsaPublicKey;
    fn new(n_bytes: &[]u8, e: u64): RsaPublicKeyResult;
}
export class RsaPublicKeyResult { ok: bool, key: RsaPublicKey, err: i64 }

export class RsaPrivateKey {
    version: u8, n: [512]u8, n_len: usize, e: u64,
    d: [512]u8, d_len: usize, p: [256]u8, p_len: usize, q: [256]u8, q_len: usize,
    dp: [256]u8, dp_len: usize, dq: [256]u8, dq_len: usize, qinv: [256]u8, qinv_len: usize,
    fn empty(): RsaPrivateKey;
    fn publicKey(): RsaPublicKey;
    fn zeroize(_: auto);
}
export class RsaPrivateKeyResult { ok: bool, key: RsaPrivateKey, err: i64 }

export fn modulusBitLen(pub_key: RsaPublicKey): usize;
export fn encodeRsaPublicKeyDer(pub_key: RsaPublicKey, out: []u8): i64;
export fn decodeRsaPublicKeyDer(der: &[]u8): RsaPublicKeyResult;
export fn encodeRsaPublicKeyPem(pub_key: RsaPublicKey, crlf: bool, out: []u8): i64;
export fn decodeRsaPublicKeyPem(pem: string): RsaPublicKeyResult;
export fn encodeRsaPrivateKeyDer(priv_key: RsaPrivateKey, out: []u8): i64;
export fn decodeRsaPrivateKeyDer(der: &[]u8): RsaPrivateKeyResult;
export fn encodeRsaPrivateKeyPem(priv_key: RsaPrivateKey, crlf: bool, out: []u8): i64;
export fn decodeRsaPrivateKeyPem(pem: string): RsaPrivateKeyResult;
```

## Constants

- `PK1_MAX_MODULUS_LEN = 512` (4096-bit), `PK1_MAX_PRIME_LEN = 256`, `PK1_VERSION_TWO_PRIME = 0`
- `PK1_PEM_PUBLIC_LABEL = "RSA PUBLIC KEY"`, `PK1_PEM_PRIVATE_LABEL = "RSA PRIVATE KEY"`
- `PK1_OK = 0`, `PK1_ERR_BUFFER_TOO_SMALL = -1`, `PK1_ERR_INVALID_DER = -2`, `PK1_ERR_INVALID_VERSION = -3`, `PK1_ERR_INVALID_INTEGER = -4`, `PK1_ERR_NEGATIVE_INTEGER = -5`, `PK1_ERR_KEY_TOO_LONG = -6`, `PK1_ERR_TRAILING_DATA = -7`, `PK1_ERR_INVALID_PEM = -8`, `PK1_ERR_INVALID_PEM_LABEL = -9`

## Pitfalls

1. **Always `defer res.key.zeroize()`**: Declare `let mut res = decodeRsaPrivateKey...` so `zeroize()` can securely wipe private key buffers via `subtle::zeroizeBytes`.
2. **Only reads/writes valid slices**: Use `key.n[0..key.n_len]` etc. — tail bytes beyond `*_len` are `undefined`.
3. **Buffer sizing**: 4096-bit private key DER needs ~2400 bytes, PEM needs ~3300 bytes; undersized `out` returns `PK1_ERR_BUFFER_TOO_SMALL (-1)`.
4. **PKCS#1 vs PKCS#8 labels**: `decodeRsaPrivateKeyPem` strictly requires `-----BEGIN RSA PRIVATE KEY-----`; for `-----BEGIN PRIVATE KEY-----` use `pkcs8`.
