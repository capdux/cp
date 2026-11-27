[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cookie-parser

Zero-heap HTTP `Cookie` header parser with HMAC-SHA256 signed cookie verification (`s:`), multi-secret key rotation (up to 4 keys), and JSON cookie (`j:`) prefix extraction in pure CP, composing `tough-cookie` and `cookie-signature`.

- **Upstream Alignment**: `npm` `cookie-parser` (MIT, expressjs/cookie-parser)
- **Package Version**: `1.0.0`
- **Memory Model**: Zero heap allocation — fixed-capacity stack `ParsedCookies` (`32` regular + `32` signed entries) and zero-copy borrowed slices.

## Installation & Import

```bash
cap add cookie-parser
```

```cp
import {
    CookieEntry,
    ParsedCookies,
    cprStartsWith2,
    isSignedCookie,
    isJsonCookie,
    stripJsonCookiePrefix,
    signCookieValue,
    signedCookie,
    signedCookieWithSecrets,
    lookupRawCookie,
    cprParseInto,
    parseCookieHeader,
    parseCookieHeaderWithSecrets,
} from "cookie-parser";
```

## Core API

| Symbol | Signature | Description |
| --- | --- | --- |
| `CookieEntry` | `class CookieEntry { name: string, value: string }` | Zero-copy cookie name-value pair borrowing from the input header. |
| `ParsedCookies` | `class ParsedCookies { cookies: [32]CookieEntry, cookie_count: usize, signed_cookies: [32]CookieEntry, signed_count: usize }` | Fixed-capacity parsed result with `ParsedCookies.empty()`, `get(name): string?`, `getSigned(name): string?`, `totalCount(): usize`. |
| `isSignedCookie` / `isJsonCookie` | `fn isSignedCookie(val: string): bool` / `fn isJsonCookie(val: string): bool` | Checks whether `val` begins with Express `"s:"` or `"j:"` prefix. |
| `stripJsonCookiePrefix` | `fn stripJsonCookiePrefix(val: string): string?` | Strips `"j:"` prefix and returns the raw JSON slice, or `null` if not a JSON cookie. |
| `signCookieValue` | `fn signCookieValue(val: string, secret: string, out: []u8): usize` | Produces `"s:<val>.<hmac-sha256>"` into `out`; returns bytes written. |
| `signedCookie` | `fn signedCookie(val: string, secret: string): string?` | Verifies an `"s:"` signed cookie value with `secret` and returns the unsigned value slice, or `null` on failure. |
| `signedCookieWithSecrets` | `fn signedCookieWithSecrets(val: string, secrets: [4]string, secret_count: usize): string?` | Verifies `"s:"` cookie against up to 4 rotated secrets in order. |
| `lookupRawCookie` | `fn lookupRawCookie(header: string, name: string): string?` | Directly extracts a raw cookie value by `name` from a `Cookie` header without populating `ParsedCookies`. |
| `parseCookieHeader` | `fn parseCookieHeader(header: string, secret: string): ParsedCookies` | Parses `Cookie` header, splitting verified `"s:"` cookies into `signed_cookies` and others into `cookies`. |
| `parseCookieHeaderWithSecrets` | `fn parseCookieHeaderWithSecrets(header: string, secrets: [4]string, secret_count: usize): ParsedCookies` | Parses `Cookie` header with multi-secret key rotation (up to 4 secrets). |
| `cprParseInto` / `cprStartsWith2` | `fn cprParseInto(header, secrets, secret_count): ParsedCookies` / `fn cprStartsWith2(s, c0, c1): bool` | Low-level parser entry point and 2-byte prefix helper. |

## Examples

### 1. Sign & Parse HTTP `Cookie` Header

```cp
import { signCookieValue, parseCookieHeader } from "cookie-parser";

fn cookieDemo(): void {
    let mut signed_buf: [128]u8 = [0; 128];
    const n = signCookieValue("user-42", "my-secret", signed_buf[0..]);
    // signed_buf[0..n] == "s:user-42.<signature>"

    const parsed = parseCookieHeader("theme=dark; lang=zh", "my-secret");
    const theme = parsed.get("theme"); // Some("dark")
}
```

### 2. Multi-Secret Key Rotation & JSON Cookie Extraction

```cp
import { parseCookieHeaderWithSecrets, stripJsonCookiePrefix } from "cookie-parser";

fn rotationDemo(header: string): void {
    const secrets: [4]string = ["new-secret", "old-secret", "", ""];
    const parsed = parseCookieHeaderWithSecrets(header, secrets, 2);
    if (let sid = parsed.getSigned("session")) {
        // verified with either "new-secret" or "old-secret"
    }
    if (let prefs = parsed.get("prefs")) {
        if (let json_str = stripJsonCookiePrefix(prefs)) {
            // raw JSON payload without "j:"
        }
    }
}
```

## Differences from Upstream & Highlights

- **Zero Heap Allocation**: Stores up to 32 regular and 32 signed cookies on the stack inside `ParsedCookies`, borrowing all names and values directly from `header`.
- **Constant-Time Verification**: Delegates HMAC-SHA256 signing/unsigning to `cookie-signature` and header lookup to `tough-cookie`.
- **Key Rotation Built In**: `parseCookieHeaderWithSecrets` and `signedCookieWithSecrets` accept `[4]string` + `secret_count` for zero-downtime secret rotation.
