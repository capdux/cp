// packages/@cap/cookie-crate/src/index.cp
// Zero-allocation Rust-style CookieBuilder, percent-encoded Cookie helpers & removal cookie generator.
// Clean-room MIT implementation inspired by Rust `cookie` v0.18; reuses @cap/cookie-jar & @cap/percent-encoding.

import {
    Cookie,
    parseSetCookie,
    serializeSetCookie,
    SAMESITE_LAX,
    SAMESITE_STRICT,
    SAMESITE_NONE,
    SAMESITE_NONE_UNSET,
} from "../../cookie-jar/src/index.cp";
import {
    componentSet,
    percentEncode,
    percentDecode,
} from "../../percent-encoding/src/index.cp";

const CKC_MAX_ENC_VAL: usize = 1024;

/// Metadata & decoded byte lengths returned by `parseEncodedCookie`.
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

fn ckcDefaultCookie(name: string, value: string): Cookie {
    return Cookie {
        name: name,
        value: value,
        domain: "",
        path: "",
        max_age: 0,
        has_max_age: false,
        secure: false,
        http_only: false,
        same_site: SAMESITE_NONE_UNSET,
    };
}

/// Fluent zero-allocation builder for constructing `Cookie` instances.
pub struct CookieBuilder {
    pub cookie: Cookie,

    /// Start building a new cookie with `name` and `value`.
    pub static fn new(name: string, value: string): CookieBuilder {
        return CookieBuilder {
            cookie: ckcDefaultCookie(name, value),
        };
    }

    /// Set the `Domain` attribute and return `*CookieBuilder` for fluent chaining.
    pub fn domain(*_, d: string): *CookieBuilder {
        _.cookie.domain = d;
        return _;
    }

    /// Set the `Path` attribute and return `*CookieBuilder` for fluent chaining.
    pub fn path(*_, p: string): *CookieBuilder {
        _.cookie.path = p;
        return _;
    }

    /// Set the `Max-Age` attribute (in seconds) and return `*CookieBuilder` for fluent chaining.
    pub fn maxAge(*_, secs: i64): *CookieBuilder {
        _.cookie.max_age = secs;
        _.cookie.has_max_age = true;
        return _;
    }

    /// Set the `Secure` flag and return `*CookieBuilder` for fluent chaining.
    pub fn secure(*_, v: bool): *CookieBuilder {
        _.cookie.secure = v;
        return _;
    }

    /// Set the `HttpOnly` flag and return `*CookieBuilder` for fluent chaining.
    pub fn httpOnly(*_, v: bool): *CookieBuilder {
        _.cookie.http_only = v;
        return _;
    }

    /// Set the `SameSite` attribute (`SAMESITE_LAX`, `SAMESITE_STRICT`, `SAMESITE_NONE`, `SAMESITE_NONE_UNSET`).
    pub fn sameSite(*_, s: u8): *CookieBuilder {
        _.cookie.same_site = s;
        return _;
    }

    /// Finalize and return the built `Cookie`.
    pub fn build(): Cookie {
        return _.cookie;
    }
}

/// Parse a `Set-Cookie` header via `@cap/cookie-jar` and percent-decode its name & value into caller buffers.
pub fn parseEncodedCookie(header: string, name_out: []u8, val_out: []u8): EncodedCookieMeta {
    const raw: Cookie = parseSetCookie(header);
    if (raw.name.len == 0) {
        return EncodedCookieMeta {
            name_len: 0,
            value_len: 0,
            domain: "",
            path: "",
            max_age: 0,
            has_max_age: false,
            secure: false,
            http_only: false,
            same_site: SAMESITE_NONE_UNSET,
            ok: false,
        };
    }
    const n_len: usize = percentDecode(raw.name, name_out);
    const v_len: usize = percentDecode(raw.value, val_out);
    return EncodedCookieMeta {
        name_len: n_len,
        value_len: v_len,
        domain: raw.domain,
        path: raw.path,
        max_age: raw.max_age,
        has_max_age: raw.has_max_age,
        secure: raw.secure,
        http_only: raw.http_only,
        same_site: raw.same_site,
        ok: true,
    };
}

/// Percent-encode `c.value` using WHATWG `componentSet()` and serialize the full `Set-Cookie` header into `out`.
/// Returns the number of bytes written to `out`.
pub fn formatEncodedCookie(c: Cookie, out: []u8): usize {
    let enc_val: [1024]u8 = undefined;
    const enc_len: usize = percentEncode(c.value, componentSet(), enc_val[0..CKC_MAX_ENC_VAL]);
    const encoded_cookie = Cookie {
        name: c.name,
        value: enc_val[0..enc_len],
        domain: c.domain,
        path: c.path,
        max_age: c.max_age,
        has_max_age: c.has_max_age,
        secure: c.secure,
        http_only: c.http_only,
        same_site: c.same_site,
    };
    return serializeSetCookie(encoded_cookie, out);
}

/// Format a standard removal `Set-Cookie` header with empty value and `Max-Age=0`.
/// Defaults `Path` to `"/"` when `path` is empty. Returns the number of bytes written to `out`.
pub fn makeRemovalCookie(name: string, path: string, out: []u8): usize {
    let p: string = path;
    if (p.len == 0) {
        p = "/";
    }
    let b = CookieBuilder.new(name, "");
    const c: Cookie = b.path(p).maxAge(0).build();
    return serializeSetCookie(c, out);
}

/// Re-exported SameSite attribute constants for convenience.
pub const COOKIE_SAMESITE_UNSET: u8 = SAMESITE_NONE_UNSET;
pub const COOKIE_SAMESITE_LAX: u8 = SAMESITE_LAX;
pub const COOKIE_SAMESITE_STRICT: u8 = SAMESITE_STRICT;
pub const COOKIE_SAMESITE_NONE: u8 = SAMESITE_NONE;
