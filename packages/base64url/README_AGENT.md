# base64url@1.0.0 (Agent Quickref)

> Zero-heap RFC 4648 Section 5 Base64URL (no-padding) encoder, decoder, and standard Base64 converter (clean-room port of npm base64url) | License: MIT
> Import: `import { ... } from "base64url";` | Full types: `src/index.d.cp`
> Deps: `data-encoding`

## Functions
- `export fn encodeBase64Url(src: &[]u8, out: []u8): usize;`
- `export fn decodeBase64Url(src: &[]u8, out: []u8): DecodeResult;`
- `export fn encodedBase64UrlLen(input_len: usize): usize;`
- `export fn fromStandardBase64(b64: string, out: []u8): usize;`
- `export fn toStandardBase64(b64url: string, out: []u8): usize;`
- `export fn isValidBase64Url(s: string): bool;`
