# `cap:quoted_printable` — RFC 2045 §6.7 Quoted-Printable Codec

Zero-allocation RFC 2045 §6.7 MIME Quoted-Printable encoder (76-column soft line wraps, trailing whitespace escaping) and strict/lenient decoder.

## Exports

- `encodeQuotedPrintable(input, out): ([]const u8)?`
- `decodeQuotedPrintable(input, out, strict): ([]const u8)?`
