# cookie-parser — AI Agent Quick Ref

Zero-heap HTTP `Cookie` header parser with HMAC-SHA256 signed cookie (`s:`), 4-key rotation, and JSON cookie (`j:`) support (npm `cookie-parser` parity).

## Import

```cp
import {
    CookieEntry, ParsedCookies,
    cprStartsWith2, isSignedCookie, isJsonCookie, stripJsonCookiePrefix,
    signCookieValue, signedCookie, signedCookieWithSecrets, lookupRawCookie,
    cprParseInto, parseCookieHeader, parseCookieHeaderWithSecrets,
} from "cookie-parser";
```

## API Signatures

```cp
export class CookieEntry { name: string, value: string }
export class ParsedCookies {
    cookies: [32]CookieEntry, cookie_count: usize,
    signed_cookies: [32]CookieEntry, signed_count: usize,
    static fn empty(): ParsedCookies;
    fn get(name: string): string?;
    fn getSigned(name: string): string?;
    fn totalCount(): usize;
}
export fn cprStartsWith2(s: string, c0: u8, c1: u8): bool;
export fn isSignedCookie(val: string): bool;
export fn isJsonCookie(val: string): bool;
export fn stripJsonCookiePrefix(val: string): string?;
export fn signCookieValue(val: string, secret: string, out: []u8): usize;
export fn signedCookie(val: string, secret: string): string?;
export fn signedCookieWithSecrets(val: string, secrets: [4]string, secret_count: usize): string?;
export fn lookupRawCookie(header: string, name: string): string?;
export fn cprParseInto(header: string, secrets: [4]string, secret_count: usize): ParsedCookies;
export fn parseCookieHeader(header: string, secret: string): ParsedCookies;
export fn parseCookieHeaderWithSecrets(header: string, secrets: [4]string, secret_count: usize): ParsedCookies;
```

## Minimal Example

```cp
import { parseCookieHeader, stripJsonCookiePrefix } from "cookie-parser";

fn demo(): void {
    const p = parseCookieHeader("a=1; cfg=j:{\"ok\":true}", "secret");
    const a = p.get("a"); // Some("1")
    if (let cfg = p.get("cfg")) {
        const raw_json = stripJsonCookiePrefix(cfg); // Some("{\"ok\":true}")
    }
}
```

## Caveats & Borrowing Rules

- **Lifetime**: Every `CookieEntry.name` / `value` inside `ParsedCookies` borrows directly from `header: string`; do not drop or overwrite `header` while reading `ParsedCookies`.
- **Fixed capacity**: `ParsedCookies` holds at most `32` unsigned cookies and `32` signed cookies; extra entries beyond 32 are ignored.
- **Key rotation**: `secrets` is a fixed `[4]string` array; pass `secret_count` in `0..=4` (empty `secret` in `parseCookieHeader` disables signature verification).
- **Buffer sizing**: `signCookieValue` writes `"s:" + val + "." + base64url(hmac-sha256)` into mutable `out: []u8` (reserve at least `val.len + 46` bytes).
