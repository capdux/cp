# base64url (AI Agent Quick Ref)

Zero-heap RFC 4648 Sec. 5 Base64URL encoder, decoder, validator, and standard Base64 converter.

## Import
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

## API Signatures
- `encode` / `encodeBase64Url(src: &[]u8, out: []u8): usize` (unpadded)
- `encodePadded(src: &[]u8, out: []u8): usize` (`=`-padded)
- `decode` / `decodeBase64Url(src: &[]u8, out: []u8): DecodeResult` (`{ ok: bool, len: usize, error_pos: usize }`)
- `encodedLen` / `encodedBase64UrlLen(input_len: usize): usize`, `encodedPaddedLen(input_len: usize): usize`
- `decodedLen(src: string): usize?` (`null` if `len % 4 == 1`)
- `fromBase64` / `fromStandardBase64(b64: string, out: []u8): usize`
- `toBase64` / `toStandardBase64(b64url: string, out: []u8): usize`
- `padString(s: string, out: []u8): usize`, `unpadString(s: string): string`
- `validate` / `isValidBase64Url(s: string): bool`, `validatePadded(s: string): bool`

## Minimal Example
```typescript
import { encode, decode } from "base64url";

fn roundtrip(src: &[]u8, enc: []u8, dec: []u8): usize {
    let n = encode(src, enc);
    let r = decode(enc[0..n], dec);
    if (r.ok) { return r.len; }
    return 0;
}
```

## Pitfalls & Borrowing Rules
- `DecodeResult` comes from `data-encoding`: always check `res.ok` before reading `out[0..res.len]`; on failure `res.error_pos` gives the offending byte index.
- Pre-size `out` using `encodedLen(src.len)` / `encodedPaddedLen(src.len)` or `decodedLen(s)`; insufficient `out` returns `0` or `res.ok == false`.
- `decodedLen(src)` returns optional `usize?`; unwrap with `if (let n = decodedLen(s))`.
- `unpadString(s)` returns a zero-copy slice borrowing `s`; do not outlive `s`.
- Pair aliases (`encode`/`encodeBase64Url`, `decode`/`decodeBase64Url`, `fromBase64`/`fromStandardBase64`, `toBase64`/`toStandardBase64`, `validate`/`isValidBase64Url`) are identical.
