# data-uri-to-buffer@1.0.0 (Agent Quickref)

> Zero-allocation RFC 2397 Data URI parser, Base64/percent decoder, and encoder for CP | License: MIT
> Import: `import { ... } from "data-uri-to-buffer";` | Full types: `src/index.d.cp`

## Functions
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn appendStr(out: []u8, pos: usize, s: string): usize;`
- `export fn isUnreservedUriByte(c: u8): bool;`
- `export fn parseDataUri(uri: string): DataUri;`
- `export fn isDataUri(s: string): bool;`
- `export fn b64Val(c: u8): u8?;`
- `export fn decodeBase64Payload(payload: string, out: []u8): ([]u8)?;`
- `export fn decodePercentPayload(payload: string, out: []u8): ([]u8)?;`
- `export fn decodeDataUri(uri: string, out: []u8): ([]u8)?;`
- `export fn encodeDataUriBase64(media_type: string, payload: []u8, out: []u8): string;`
- `export fn encodeDataUriText(media_type: string, text: string, out: []u8): string;`
