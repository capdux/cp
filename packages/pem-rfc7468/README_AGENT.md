# pem-rfc7468@1.0.0 (Agent Quickref)

> Zero-allocation RFC 7468 PEM encoder and strict decoder reusing @cap/base64ct for constant-time Base64 (RustCrypto pem-rfc7468 parity) in pure CP. | License: MIT
> Import: `import { ... } from "pem-rfc7468";` | Full types: `src/index.d.cp`
> Deps: `base64ct`

## Functions
- `export fn pemIsLabelChar(ch: u8): bool;`
- `export fn pemIsLabelValidBytes(s: string, start: usize, len: usize): bool;`
- `export fn isValidLabel(label: string): bool;`
- `export fn pemStartsWithAt(s: string, pos: usize, prefix: string): bool;`
- `export fn pemWriteBytes(out: []u8, pos: usize, src: string): usize;`
- `export fn pemWriteNewline(out: []u8, pos: usize, crlf: bool): usize;`
- `export fn pemAdvanceLineEnd(s: string, pos: usize): usize;`
- `export fn pemSkipLineEnding(s: string, pos: usize): usize;`
- `export fn pemFindDashes(s: string, start: usize, line_end: usize): i64;`
- `export fn pemOnlySpacesTabs(s: string, start: usize, end: usize): bool;`
- `export fn pemSliceEq(s: string, a_start: usize, a_len: usize, b_start: usize, b_len: usize): bool;`
- `export fn pemEncodedLen(label_len: usize, der_len: usize, crlf: bool): usize;`
- `export fn encodePem(label: string, der: &[]u8, line_ending_crlf: bool, out: []u8): i64;`
- `export fn locatePem(input: string): PemSlice;`
- `export fn decodeLabel(input: string, label_out: []u8): i64;`
- `export fn decodePem(input: string, label_out: []u8, label_len_out: &mut usize, der_out: []u8): i64;`
