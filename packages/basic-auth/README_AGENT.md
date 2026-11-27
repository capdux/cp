# basic-auth@1.0.0 (Agent Quickref)

> Zero-heap RFC 7617 HTTP Basic Authentication parser, encoder, constant-time verifier, and WWW-Authenticate challenge formatter. | License: MIT
> Import: `import { ... } from "basic-auth";` | Full types: `src/index.d.cp`

## Functions
- `export fn isWs(c: u8): bool;`
- `export fn toLowerAscii(c: u8): u8;`
- `export fn b64EncChar(v: u8): u8;`
- `export fn b64DecVal(c: u8): i32;`
- `export fn b64Encode(src: &[]u8, dst: []u8): i64;`
- `export fn b64Decode(src: &[]u8, dst: []u8): i64;`
- `export fn decodeHeader(header: string, out_user: []u8, out_pass: []u8): BasicCredentials;`
- `export fn encodeHeader(user: string, pass: string, out: []u8): usize;`
- `export fn constantTimeEq(a: &[]u8, b: &[]u8): bool;`
- `export fn verifyHeader(header: string, expected_user: string, expected_pass: string): bool;`
- `export fn appendSlice(out: []u8, pos: usize, s: string): i64;`
- `export fn formatChallenge(realm: string, utf8: bool, out: []u8): usize;`
