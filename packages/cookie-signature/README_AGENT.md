# cookie-signature — AI Agent Quick Ref

- **Import**: `import { SAME_SITE_OMIT, SAME_SITE_STRICT, SAME_SITE_LAX, SAME_SITE_NONE, CookieSlice, CookieOptions, UnsignResult, Sha256State, sign, unsign, getCookie, formatSetCookie, sha256, hmacSha256, ctEqual, encodeBase64Url32 } from "cookie-signature";`
- **Source**: npm `cookie-signature` (`v1.0.0`, zero-heap HMAC-SHA256 + RFC 6265 Cookie codec)

## Constants
```cp
SAME_SITE_OMIT: u8 = 0
SAME_SITE_STRICT: u8 = 1
SAME_SITE_LAX: u8 = 2
SAME_SITE_NONE: u8 = 3
```

## Core Signatures
```cp
class CookieSlice { start: usize, len: usize, found: bool }
class UnsignResult { ok: bool, val_len: usize }
class CookieOptions {
    max_age: i64, path: &[]u8, domain: &[]u8,
    http_only: bool, secure: bool, same_site: u8,
}

sign(val: &[]u8, secret: &[]u8, dst: []u8): usize;        // 0 if dst.len < val.len + 44
unsign(signed_val: &[]u8, secret: &[]u8): UnsignResult;   // constant-time verify
getCookie(header: &[]u8, name: &[]u8): CookieSlice;
formatSetCookie(name: &[]u8, value: &[]u8, opts: CookieOptions, dst: []u8): i64; // -1 on overflow

// Crypto primitives
sha256(data: &[]u8, out32: []u8);
hmacSha256(key: &[]u8, msg: &[]u8, out: []u8);            // out.len >= 32
ctEqual(a: &[]u8, b: &[]u8): bool;
encodeBase64Url32(mac: &[]u8, dst: []u8);                 // dst.len >= 43
sha256Init(): Sha256State;
sha256Update(st: &mut Sha256State, data: &[]u8);
sha256Final(st: &mut Sha256State, out32: []u8);
```

## Pitfalls
- `sign` outputs `<val>.<43-byte base64url MAC>`: ensure `dst.len >= val.len + 44`, otherwise `sign` returns `0`.
- `unsign` does not copy; when `res.ok` is true, slice the original value via `signed_val[0..res.val_len]`.
- `formatSetCookie` omits `Max-Age` when `opts.max_age < 0` and omits `Path`/`Domain` when empty; returns `-1` if `dst` is too small.
- `unsign` uses `ctEqual` internally to prevent timing side-channels.
