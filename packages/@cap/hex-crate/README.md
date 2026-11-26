# @cap/hex-crate

Clean-room MIT, zero-heap hexadecimal encoding, decoding, and validation library for CP — modeled on Rust's `hex` crate (1.5B+ downloads).

## Dependencies
- `@cap/data-encoding` (`1.0.0`) — reused for RFC 4648 Base16/Hex encoding and decoding.

## API
- `encodeHex(src, out): usize` & `encodeHexUpper(src, out): usize`
- `decodeHex(src, out): DecodeResult` & `decodeHexPrefixed(src, out): DecodeResult`
- `hexEncodedLen(n): usize`, `hexDecodedLen(n): usize?`, `isValidHex(s): bool`
