# `@cap/cookie-crate`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:cookie` ([https://github.com/SergioBenitez/cookie-rs](https://github.com/SergioBenitez/cookie-rs))（原库名 `cookie` → CP 包名 `@cap/cookie-crate`）

Zero-allocation Rust-style `CookieBuilder`, percent-encoded `Set-Cookie` parser/formatter, and removal cookie generator for CP (clean-room MIT, inspired by Rust `cookie` v0.18).

## Dependencies (Zero Duplication)

- `@cap/cookie-jar`: reuses `Cookie`, `parseSetCookie`, `serializeSetCookie`, and `SAMESITE_*` constants.
- `@cap/percent-encoding`: reuses `componentSet`, `percentEncode`, and `percentDecode`.

## API

```cp
pub struct EncodedCookieMeta {
    pub name_len: usize,
    pub value_len: usize,
    pub domain: string,
    pub path: string,
    pub max_age: i64,
    pub has_max_age: bool,
    pub secure: bool,
    pub http_only: bool,
    pub same_site: u8,
    pub ok: bool,
}

pub struct CookieBuilder {
    pub static fn new(name: string, value: string): CookieBuilder
    pub fn domain(*_, d: string): *CookieBuilder
    pub fn path(*_, p: string): *CookieBuilder
    pub fn maxAge(*_, secs: i64): *CookieBuilder
    pub fn secure(*_, v: bool): *CookieBuilder
    pub fn httpOnly(*_, v: bool): *CookieBuilder
    pub fn sameSite(*_, s: u8): *CookieBuilder
    pub fn build(): Cookie
}

pub fn parseEncodedCookie(header: string, name_out: []u8, val_out: []u8): EncodedCookieMeta
pub fn formatEncodedCookie(c: Cookie, out: []u8): usize
pub fn makeRemovalCookie(name: string, path: string, out: []u8): usize
```

## Usage

```cp
import {
    CookieBuilder,
    COOKIE_SAMESITE_LAX,
    formatEncodedCookie,
    parseEncodedCookie,
    makeRemovalCookie,
} from "@cap/cookie-crate";

fn example() {
    let b = CookieBuilder.new("session", "a b/c=d");
    const c = b.domain("example.com").path("/app").maxAge(3600).secure(true).httpOnly(true).sameSite(COOKIE_SAMESITE_LAX).build();

    let hdr: [256]u8 = undefined;
    const n = formatEncodedCookie(c, hdr[0..256]);
    // "session=a%20b%2Fc%3Dd; Domain=example.com; Path=/app; Max-Age=3600; Secure; HttpOnly; SameSite=Lax"
}
```

## License

MIT
