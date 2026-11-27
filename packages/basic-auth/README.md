[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# basic-auth

Zero-heap RFC 7617 HTTP Basic Authentication header parser, encoder, constant-time verifier, and `WWW-Authenticate` challenge formatter in pure CP.

- **Upstream Alignment**: `npm` `basic-auth` (jshttp/basic-auth) & `crates.io` `http-auth-basic` (MIT)
- **Package Version**: `1.0.0`
- **Memory Model**: Zero heap allocation — caller-supplied `[]u8` buffers and constant-time byte comparison via `cap:bin.ctEqual`.

## Installation & Import

```bash
cap add basic-auth
```

```cp
import {
    BasicCredentials,
    isWs,
    b64EncChar,
    b64DecVal,
    b64Encode,
    b64Decode,
    decodeHeader,
    encodeHeader,
    constantTimeEq,
    verifyHeader,
    appendSlice,
    formatChallenge,
} from "basic-auth";
```

## Core API

| Symbol | Signature | Description |
| --- | --- | --- |
| `BasicCredentials` | `class BasicCredentials { user_len: usize, pass_len: usize, valid: bool }` | Result of `decodeHeader`, reporting decoded username/password lengths written into `out_user` / `out_pass` and validity. |
| `decodeHeader` | `fn decodeHeader(header: string, out_user: []u8, out_pass: []u8): BasicCredentials` | Parses `"Basic <base64>"` (case-insensitive scheme), decodes `user:pass` into caller buffers, and returns `BasicCredentials`. |
| `encodeHeader` | `fn encodeHeader(user: string, pass: string, out: []u8): usize` | Formats `"Basic <base64(user:pass)>"` into `out`; returns bytes written (`0` if buffer too small). |
| `verifyHeader` | `fn verifyHeader(header: string, expected_user: string, expected_pass: string): bool` | Decodes `header` and verifies both username and password in constant time. |
| `formatChallenge` | `fn formatChallenge(realm: string, utf8: bool, out: []u8): usize` | Formats RFC 7617 `Basic realm="..."` (plus `, charset="UTF-8"` when `utf8 == true`) into `out`. |
| `constantTimeEq` | `fn constantTimeEq(a: &[]u8, b: &[]u8): bool` | Constant-time equality comparison for secrets/credentials. |
| `b64Encode` / `b64Decode` | `fn b64Encode(src: &[]u8, dst: []u8): i64` / `fn b64Decode(src: &[]u8, dst: []u8): i64` | Standard RFC 4648 Base64 encode/decode into `dst`; returns bytes written or `-1` on error. |
| `isWs` / `b64EncChar` / `b64DecVal` / `appendSlice` | Low-level helpers | ASCII whitespace check, Base64 table lookups, and safe slice append (`-1` on overflow). |

## Examples

### 1. Encode & Verify `Authorization` Header

```cp
import { encodeHeader, verifyHeader } from "basic-auth";

fn authDemo(): void {
    let mut hdr_buf: [128]u8 = [0; 128];
    const n = encodeHeader("alice", "s3cr3t", hdr_buf[0..]);
    // hdr_buf[0..n] == "Basic YWxpY2U6czNjcjN0"

    const ok = verifyHeader("Basic YWxpY2U6czNjcjN0", "alice", "s3cr3t"); // true
}
```

### 2. Decode Credentials into Stack Buffers

```cp
import { decodeHeader } from "basic-auth";

fn decodeDemo(auth_header: string): void {
    let mut user_buf: [64]u8 = [0; 64];
    let mut pass_buf: [64]u8 = [0; 64];
    const creds = decodeHeader(auth_header, user_buf[0..], pass_buf[0..]);
    if (creds.valid) {
        // user_buf[0..creds.user_len], pass_buf[0..creds.pass_len]
    }
}
```

### 3. Format `WWW-Authenticate` Challenge Header

```cp
import { formatChallenge } from "basic-auth";

fn challengeDemo(): void {
    let mut buf: [128]u8 = [0; 128];
    const n = formatChallenge("Admin Area", true, buf[0..]);
    // buf[0..n] == `Basic realm="Admin Area", charset="UTF-8"`
}
```

## Differences from Upstream & Highlights

- **Constant-Time Verification**: Built-in `verifyHeader` and `constantTimeEq` (backed by `cap:bin.ctEqual`) prevent timing side-channel leaks during credential checks.
- **RFC 7617 Complete**: Covers both parsing (`decodeHeader`) and response generation (`encodeHeader`, `formatChallenge` with optional `charset="UTF-8"`).
- **Zero Heap Allocation**: All Base64 decoding and formatting run inside caller-owned stack buffers, making it safe to zeroize password buffers immediately after use.
