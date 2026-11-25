// Type definitions for CP

export error CookieError { InvalidName, InvalidValue, InvalidAttribute, BufferTooSmall }

export enum SameSite {
    Unset = 0,
    Strict = 1,
    Lax = 2,
    None = 3
}

export class CookiePair {
    name: &[]u8,
    value: &[]u8,
}

export class SetCookieOptions {
    path: &[]u8,
    domain: &[]u8,
    expires: &[]u8,
    max_age: i64?,
    http_only: bool,
    secure: bool,
    partitioned: bool,
    same_site: SameSite,
}

export fn isTokenChar(c: u8): bool;
export fn isCookieOctet(c: u8): bool;
export fn isAttrSafe(s: &[]u8): bool;
export fn isValidCookieName(name: &[]u8): bool;
export fn isValidCookieValue(val: &[]u8): bool;
export fn trimOws(s: &[]u8): &[]u8;
export class CookieIter {
    src: &[]u8,
    pos: usize,
    fn init(header: &[]u8): CookieIter;
    fn next(_: auto): CookiePair?;
}

export fn getCookie(header: &[]u8, name: &[]u8): (&[]u8)?;
export fn countCookies(header: &[]u8): usize;
export fn appendStr(out: []u8, pos: &mut usize, s: &[]u8): bool;
export fn serializeSetCookie(name: &[]u8, value: &[]u8, opts: SetCookieOptions, out: []u8): &[]u8 ! CookieError;
export fn serializeClearCookie(name: &[]u8, path: &[]u8, out: []u8): &[]u8 ! CookieError;
