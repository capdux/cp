# express-bearer-token — AI Agent Quick Ref

Zero-allocation RFC 6750 HTTP Bearer token extractor, JWT structure precheck, constant-time verifier, and WWW-Authenticate challenge formatter in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    isAuthSpace,
    isB64TokenCore,
    isValidTokenChars,
    extractBearer,
    formatBearer,
    isBase64UrlChar,
    isJwtLike,
    constantTimeEq,
} from "express-bearer-token";
```

## API Signatures

```cp
fn isAuthSpace(b: u8): bool;
fn isB64TokenCore(b: u8): bool;
fn isValidTokenChars(token: string): bool;
fn extractBearer(header: string): string;
fn formatBearer(token: string, out: []u8): usize;
fn isBase64UrlChar(b: u8): bool;
fn isJwtLike(token: string): bool;
fn constantTimeEq(a: string, b: string): bool;
fn verifyBearer(header: string, expected_token: string): bool;
fn pushSlice(dst: []u8, pos: usize, src: string): i64;
fn pushQuotedParam(dst: []u8, pos: usize, first: bool, key: string, val: string): i64;
fn formatChallenge(realm: string, err_code: string, err_desc: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { isValidTokenChars } from "express-bearer-token";

pub fn main() {
    let result = isValidTokenChars("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"express-bearer-token"`; install via `cap add express-bearer-token`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
