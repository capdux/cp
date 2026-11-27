# basic-auth — AI Agent Quick Ref

Zero-heap RFC 7617 HTTP Basic Authentication parser, encoder, constant-time verifier, and `WWW-Authenticate` formatter (npm `basic-auth` / Rust `http-auth-basic` parity).

## Import

```cp
import {
    BasicCredentials, isWs, b64EncChar, b64DecVal, b64Encode, b64Decode,
    decodeHeader, encodeHeader, constantTimeEq, verifyHeader,
    appendSlice, formatChallenge,
} from "basic-auth";
```

## API Signatures

```cp
export class BasicCredentials {
    user_len: usize, pass_len: usize, valid: bool,
}
export fn isWs(c: u8): bool;
export fn b64EncChar(v: u8): u8;
export fn b64DecVal(c: u8): i32;
export fn b64Encode(src: &[]u8, dst: []u8): i64;
export fn b64Decode(src: &[]u8, dst: []u8): i64;
export fn decodeHeader(header: string, out_user: []u8, out_pass: []u8): BasicCredentials;
export fn encodeHeader(user: string, pass: string, out: []u8): usize;
export fn constantTimeEq(a: &[]u8, b: &[]u8): bool;
export fn verifyHeader(header: string, expected_user: string, expected_pass: string): bool;
export fn appendSlice(out: []u8, pos: usize, s: string): i64;
export fn formatChallenge(realm: string, utf8: bool, out: []u8): usize;
```

## Minimal Example

```cp
import { encodeHeader, decodeHeader, verifyHeader } from "basic-auth";

fn demo(): void {
    let mut hdr: [128]u8 = [0; 128];
    const n = encodeHeader("alice", "pass", hdr[0..]);
    let mut u: [64]u8 = [0; 64];
    let mut p: [64]u8 = [0; 64];
    const c = decodeHeader("Basic YWxpY2U6cGFzcw==", u[0..], p[0..]); // c.valid == true
    const ok = verifyHeader("Basic YWxpY2U6cGFzcw==", "alice", "pass"); // true
}
```

## Caveats & Borrowing Rules

- **Check `creds.valid`**: Always verify `creds.valid == true` before slicing `out_user[0..creds.user_len]` and `out_pass[0..creds.pass_len]`.
- **Read-only `&[]u8` vs mutable `[]u8`**: `b64Encode`, `b64Decode`, and `constantTimeEq` take read-only `&[]u8` inputs; `dst`/`out`/`out_user`/`out_pass` require mutable `[]u8` stack buffers.
- **Error sentinels**: `b64Encode`, `b64Decode`, and `appendSlice` return `-1` on invalid Base64 or insufficient destination buffer; `encodeHeader` and `formatChallenge` return `0` on overflow.
- **Security**: Prefer `verifyHeader` over manual `decodeHeader` + `==` so credentials are compared in constant time.
