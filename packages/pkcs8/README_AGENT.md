# pkcs8 (AI Agent Quick Ref)

- **Import**: `import { PrivateKeyInfo, Pkcs8DecodeResult, encodePkcs8Der, decodePkcs8Der, encodePkcs8Pem, decodePkcs8Pem, PK8_VERSION_V1, PK8_VERSION_V2, PK8_PEM_LABEL, PK8_OK } from "pkcs8";`
- **Source**: `crates.io:pkcs8` (MIT, zero-alloc RFC 5958/8410, wraps `spki` + `const-oid` + `pem-rfc7468` + `subtle`).

## Core Signatures

```cp
export class PrivateKeyInfo {
    version: u8,
    algorithm: AlgorithmIdentifier,
    private_key: [160]u8, private_key_len: usize,
    has_public_key: bool,
    public_key: [64]u8, public_key_len: usize,
    fn empty(): PrivateKeyInfo;
    fn ed25519(seed32: &[]u8): Pkcs8DecodeResult;
    fn ed25519V2(seed32: &[]u8, pub32: &[]u8): Pkcs8DecodeResult;
    fn p256(ec_priv_der: &[]u8): Pkcs8DecodeResult;
    fn zeroize(_: auto);
}

export class Pkcs8DecodeResult {
    ok: bool, info: PrivateKeyInfo, err: i64,
}

export fn encodePkcs8Der(info: PrivateKeyInfo, out: []u8): i64;
export fn decodePkcs8Der(der: &[]u8): Pkcs8DecodeResult;
export fn encodePkcs8Pem(info: PrivateKeyInfo, crlf: bool, out: []u8): i64;
export fn decodePkcs8Pem(pem: string): Pkcs8DecodeResult;
```

## Constants

- `PK8_VERSION_V1 = 0`, `PK8_VERSION_V2 = 1`, `PK8_MAX_PRIVKEY_LEN = 160`, `PK8_MAX_PUBKEY_LEN = 64`, `PK8_PEM_LABEL = "PRIVATE KEY"`
- `PK8_OK = 0`, `PK8_ERR_BUFFER_TOO_SMALL = -1`, `PK8_ERR_INVALID_VERSION = -2`, `PK8_ERR_INVALID_DER = -3`, `PK8_ERR_INVALID_TAG = -4`, `PK8_ERR_TRUNCATED = -5`, `PK8_ERR_INVALID_ALG = -6`, `PK8_ERR_KEY_TOO_LONG = -7`, `PK8_ERR_INVALID_PUBLIC_KEY = -8`, `PK8_ERR_INVALID_PEM = -9`, `PK8_ERR_INVALID_PEM_LABEL = -10`, `PK8_ERR_TRAILING_DATA = -11`, `PK8_ERR_INVALID_SEED = -12`

## Minimal Pattern

```cp
let mut res = decodePkcs8Pem(pem);
if (!res.ok) { return res.err; }
defer res.info.zeroize();
let priv_bytes = res.info.private_key[0..res.info.private_key_len];
```

## Pitfalls

1. **Always `defer res.info.zeroize()`**: Declare `let mut res` so `zeroize()` can wipe `private_key` and `public_key` via `subtle::zeroizeBytes`.
2. **Capacity limit**: `private_key` holds up to 160 bytes (ideal for Ed25519 / P-256 / Curve25519); oversized RSA PKCS#8 keys return `PK8_ERR_KEY_TOO_LONG (-7)` (use `pkcs1` for RSA).
3. **Seed length**: `ed25519` requires `seed32.len == 32`, `ed25519V2` requires both `seed32.len == 32` and `pub32.len == 32`; otherwise returns `PK8_ERR_INVALID_SEED (-12)`.
4. **PEM label**: `decodePkcs8Pem` strictly requires `-----BEGIN PRIVATE KEY-----` (unencrypted PKCS#8).
