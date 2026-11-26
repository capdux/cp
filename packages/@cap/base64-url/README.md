# @cap/base64-url

Clean-room MIT, zero-heap RFC 4648 Section 5 Base64URL (unpadded) encoder, decoder, and standard Base64 converter for CP — modeled on npm's `base64url` v3.

## Dependencies
- `@cap/data-encoding` (`1.0.0`) — reused for RFC 4648 Base64URL encoding/decoding.

## API
- `encodeBase64Url(src, out): usize`
- `decodeBase64Url(src, out): DecodeResult`
- `encodedBase64UrlLen(input_len): usize`
- `fromStandardBase64(b64, out): usize` & `toStandardBase64(b64url, out): usize`
- `isValidBase64Url(s): bool`
